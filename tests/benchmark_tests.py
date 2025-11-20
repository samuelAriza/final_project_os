#!/usr/bin/env python3
"""
GSEA Benchmark and Resource Management Test Suite
Universidad EAFIT - Sistemas Operativos

Comprehensive testing framework for:
- Memory leak detection
- File descriptor tracking
- Process management (zombie detection)
- CPU/Memory profiling
- Performance benchmarking across algorithm combinations
- Professional visualization and reporting
"""

import os
import sys
import csv
import time
import subprocess
import argparse
import tempfile
import shutil
import random
import string
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from datetime import datetime

try:
    import psutil
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    from tqdm import tqdm
except ImportError as e:
    print(f"Error: Missing required dependency: {e}")
    print("Install with: pip3 install psutil matplotlib tqdm")
    sys.exit(1)


@dataclass
class BenchmarkResult:
    """Data class to store benchmark results"""
    compression_algorithm: str
    encryption_algorithm: str
    num_files: int
    file_size: int  # in bytes
    cpu_percent: float
    memory_mb: float
    time_seconds: float
    leaks_detected: bool
    zombies_detected: bool
    exit_code: int
    original_size_mb: float
    compressed_size_mb: float
    compression_ratio: float
    throughput_mbps: float


class ResourceMonitor:
    """Monitor system resources for a process"""
    
    def __init__(self, pid: int):
        self.pid = pid
        try:
            self.process = psutil.Process(pid)
        except psutil.NoSuchProcess:
            self.process = None
    
    def get_cpu_percent(self, interval: float = 0.1) -> float:
        """Get CPU usage percentage"""
        if self.process and self.process.is_running():
            return self.process.cpu_percent(interval=interval)
        return 0.0
    
    def get_memory_mb(self) -> float:
        """Get memory usage in MB"""
        if self.process and self.process.is_running():
            return self.process.memory_info().rss / 1024 / 1024
        return 0.0
    
    def get_open_files_count(self) -> int:
        """Get count of open file descriptors"""
        if self.process and self.process.is_running():
            try:
                return len(self.process.open_files())
            except (psutil.AccessDenied, psutil.NoSuchProcess):
                return 0
        return 0
    
    def check_zombies(self) -> bool:
        """Check for zombie child processes"""
        if not self.process or not self.process.is_running():
            return False
        
        try:
            children = self.process.children(recursive=True)
            for child in children:
                if child.status() == psutil.STATUS_ZOMBIE:
                    return True
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
        
        return False


class TestDataGenerator:
    """Generate test data with various patterns"""
    
    @staticmethod
    def random_data(size: int) -> bytes:
        """Generate random binary data"""
        return os.urandom(size)
    
    @staticmethod
    def text_data(size: int) -> bytes:
        """Generate text data (good for compression)"""
        words = ["lorem", "ipsum", "dolor", "sit", "amet", "consectetur", 
                 "adipiscing", "elit", "sed", "do", "eiusmod", "tempor"]
        text = " ".join(random.choices(words, k=size // 6))
        return text[:size].encode('utf-8')
    
    @staticmethod
    def repetitive_data(size: int) -> bytes:
        """Generate highly repetitive data (best for compression)"""
        pattern = b"ABCDEFGH" * 128
        repetitions = (size // len(pattern)) + 1
        return (pattern * repetitions)[:size]
    
    @staticmethod
    def create_test_files(directory: Path, num_files: int, file_size: int, 
                         pattern: str = "random") -> List[Path]:
        """Create test files in directory"""
        directory.mkdir(parents=True, exist_ok=True)
        files = []
        
        generator_map = {
            "random": TestDataGenerator.random_data,
            "text": TestDataGenerator.text_data,
            "repetitive": TestDataGenerator.repetitive_data
        }
        
        data_generator = generator_map.get(pattern, TestDataGenerator.random_data)
        
        for i in range(num_files):
            file_path = directory / f"testfile_{i:04d}.dat"
            data = data_generator(file_size)
            file_path.write_bytes(data)
            files.append(file_path)
        
        return files


class ValgrindAnalyzer:
    """Analyze valgrind output for memory leaks"""
    
    @staticmethod
    def run_valgrind(binary: str, args: List[str], timeout: int = 300) -> Tuple[bool, str]:
        """
        Run valgrind and detect memory leaks
        Returns: (has_leaks, output)
        """
        valgrind_cmd = [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--track-origins=yes",
            "--error-exitcode=42",
            binary
        ] + args
        
        try:
            result = subprocess.run(
                valgrind_cmd,
                capture_output=True,
                text=True,
                timeout=timeout
            )
            
            output = result.stderr  # Valgrind outputs to stderr
            
            # Check for leak summary
            has_leaks = False
            if "definitely lost:" in output:
                # Extract the bytes lost
                for line in output.split('\n'):
                    if "definitely lost:" in line:
                        bytes_lost = line.split()[3].replace(',', '')
                        if bytes_lost != '0':
                            has_leaks = True
                            break
            
            return has_leaks, output
            
        except subprocess.TimeoutExpired:
            return False, "Timeout expired"
        except FileNotFoundError:
            print("Warning: valgrind not found, skipping memory leak detection")
            return False, "valgrind not available"


class GSEABenchmark:
    """Main benchmark suite for GSEA project"""
    
    def __init__(self, binary_path: str, results_dir: str = "benchmark_results"):
        self.binary_path = Path(binary_path)
        self.results_dir = Path(results_dir)
        self.results_dir.mkdir(parents=True, exist_ok=True)
        
        # Create subdirectories
        self.plots_dir = self.results_dir / "plots"
        self.plots_dir.mkdir(exist_ok=True)
        
        self.csv_dir = self.results_dir / "csv"
        self.csv_dir.mkdir(exist_ok=True)
        
        self.logs_dir = self.results_dir / "logs"
        self.logs_dir.mkdir(exist_ok=True)
        
        # Verify binary exists
        if not self.binary_path.exists():
            raise FileNotFoundError(f"Binary not found: {self.binary_path}")
        
        # Algorithm configurations
        self.compression_algorithms = ["lz77", "huffman", "rle", "lzw"]
        self.encryption_algorithms = ["aes128", "chacha20", "salsa20", "rc4"]
        
        # Results storage
        self.results: List[BenchmarkResult] = []
    
    def run_single_test(self, comp_alg: str, enc_alg: str, input_dir: Path, 
                       output_dir: Path, num_files: int, file_size: int,
                       use_valgrind: bool = False) -> BenchmarkResult:
        """Run a single benchmark test"""
        
        # Prepare command
        cmd = [
            str(self.binary_path),
            "-ce",  # Compress and encrypt
            "--comp-alg", comp_alg,
            "--enc-alg", enc_alg,
            "-i", str(input_dir),
            "-o", str(output_dir),
            "-k", "benchmark_test_key_123",
            "-t", "4"  # Use 4 threads
        ]
        
        # Calculate original size
        original_size = num_files * file_size
        original_size_mb = original_size / 1024 / 1024
        
        # Run with valgrind if requested
        if use_valgrind:
            has_leaks, valgrind_output = ValgrindAnalyzer.run_valgrind(
                str(self.binary_path), cmd[1:]
            )
            
            # Save valgrind output
            log_file = self.logs_dir / f"valgrind_{comp_alg}_{enc_alg}.log"
            log_file.write_text(valgrind_output)
        else:
            has_leaks = False
        
        # Start monitoring
        start_time = time.time()
        
        try:
            # Run the process
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
            
            # Monitor resources
            monitor = ResourceMonitor(process.pid)
            
            cpu_samples = []
            mem_samples = []
            
            # Monitor while running
            while process.poll() is None:
                cpu_samples.append(monitor.get_cpu_percent())
                mem_samples.append(monitor.get_memory_mb())
                time.sleep(0.1)
            
            # Wait for completion
            stdout, stderr = process.communicate(timeout=300)
            exit_code = process.returncode
            
            # Check for zombies
            zombies = monitor.check_zombies()
            
        except subprocess.TimeoutExpired:
            process.kill()
            exit_code = -1
            cpu_samples = [0]
            mem_samples = [0]
            zombies = False
        
        end_time = time.time()
        elapsed_time = end_time - start_time
        
        # Calculate metrics
        avg_cpu = sum(cpu_samples) / len(cpu_samples) if cpu_samples else 0
        avg_mem = sum(mem_samples) / len(mem_samples) if mem_samples else 0
        
        # Calculate compressed size
        compressed_size = 0
        if output_dir.exists():
            for file in output_dir.rglob("*"):
                if file.is_file():
                    compressed_size += file.stat().st_size
        
        compressed_size_mb = compressed_size / 1024 / 1024
        compression_ratio = (1 - compressed_size / original_size) * 100 if original_size > 0 else 0
        throughput_mbps = original_size_mb / elapsed_time if elapsed_time > 0 else 0
        
        return BenchmarkResult(
            compression_algorithm=comp_alg,
            encryption_algorithm=enc_alg,
            num_files=num_files,
            file_size=file_size,
            cpu_percent=avg_cpu,
            memory_mb=avg_mem,
            time_seconds=elapsed_time,
            leaks_detected=has_leaks,
            zombies_detected=zombies,
            exit_code=exit_code,
            original_size_mb=original_size_mb,
            compressed_size_mb=compressed_size_mb,
            compression_ratio=compression_ratio,
            throughput_mbps=throughput_mbps
        )
    
    def run_benchmarks(self, file_configs: List[Tuple[int, int]], 
                      data_pattern: str = "text", use_valgrind: bool = False):
        """
        Run comprehensive benchmarks
        
        Args:
            file_configs: List of (num_files, file_size_bytes) tuples
            data_pattern: "random", "text", or "repetitive"
            use_valgrind: Whether to run valgrind (slow!)
        """
        
        total_tests = (len(self.compression_algorithms) * 
                      len(self.encryption_algorithms) * 
                      len(file_configs))
        
        print(f"\n{'='*70}")
        print(f"  GSEA Benchmark Suite")
        print(f"{'='*70}")
        print(f"  Binary: {self.binary_path}")
        print(f"  Compression algorithms: {', '.join(self.compression_algorithms)}")
        print(f"  Encryption algorithms: {', '.join(self.encryption_algorithms)}")
        print(f"  File configurations: {len(file_configs)}")
        print(f"  Total tests: {total_tests}")
        print(f"  Valgrind enabled: {use_valgrind}")
        print(f"{'='*70}\n")
        
        with tqdm(total=total_tests, desc="Running benchmarks", unit="test") as pbar:
            for num_files, file_size in file_configs:
                # Create temporary directories
                temp_base = tempfile.mkdtemp(prefix="gsea_bench_")
                temp_base_path = Path(temp_base)
                
                try:
                    # Generate test data
                    input_dir = temp_base_path / "input"
                    TestDataGenerator.create_test_files(
                        input_dir, num_files, file_size, data_pattern
                    )
                    
                    # Test all algorithm combinations
                    for comp_alg in self.compression_algorithms:
                        for enc_alg in self.encryption_algorithms:
                            output_dir = temp_base_path / f"output_{comp_alg}_{enc_alg}"
                            output_dir.mkdir(exist_ok=True)
                            
                            pbar.set_description(
                                f"Testing {comp_alg}+{enc_alg} "
                                f"({num_files} files × {file_size//1024}KB)"
                            )
                            
                            result = self.run_single_test(
                                comp_alg, enc_alg, input_dir, output_dir,
                                num_files, file_size, use_valgrind
                            )
                            
                            self.results.append(result)
                            pbar.update(1)
                
                finally:
                    # Cleanup temporary files
                    shutil.rmtree(temp_base, ignore_errors=True)
        
        print(f"\n✅ Benchmarks complete! {len(self.results)} tests executed.\n")
    
    def save_results_csv(self, filename: str = "benchmark_results.csv"):
        """Save results to CSV file"""
        csv_path = self.csv_dir / filename
        
        if not self.results:
            print("No results to save")
            return
        
        with open(csv_path, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=asdict(self.results[0]).keys())
            writer.writeheader()
            for result in self.results:
                writer.writerow(asdict(result))
        
        print(f"📊 Results saved to: {csv_path}")
    
    def generate_plots(self):
        """Generate professional visualization plots"""
        if not self.results:
            print("No results to plot")
            return
        
        print("\n📈 Generating visualizations...")
        
        # Set style
        plt.style.use('seaborn-v0_8-darkgrid')
        
        # 1. Memory usage vs number of files
        self._plot_memory_vs_files()
        
        # 2. Time vs file size
        self._plot_time_vs_filesize()
        
        # 3. Algorithm comparison (compression ratio)
        self._plot_compression_comparison()
        
        # 4. Throughput comparison
        self._plot_throughput_comparison()
        
        # 5. Resource efficiency heatmap
        self._plot_resource_heatmap()
        
        print(f"✅ Plots saved to: {self.plots_dir}/")
    
    def _plot_memory_vs_files(self):
        """Plot memory usage vs number of files"""
        fig, ax = plt.subplots(figsize=(12, 6))
        
        # Group by algorithm combination
        for comp_alg in self.compression_algorithms:
            for enc_alg in self.encryption_algorithms:
                filtered = [r for r in self.results 
                           if r.compression_algorithm == comp_alg 
                           and r.encryption_algorithm == enc_alg]
                
                if not filtered:
                    continue
                
                x = [r.num_files for r in filtered]
                y = [r.memory_mb for r in filtered]
                
                ax.plot(x, y, marker='o', label=f"{comp_alg}+{enc_alg}")
        
        ax.set_xlabel('Number of Files', fontsize=12)
        ax.set_ylabel('Memory Usage (MB)', fontsize=12)
        ax.set_title('Memory Usage vs Number of Files', fontsize=14, fontweight='bold')
        ax.legend(loc='best', fontsize=9)
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'memory_vs_files.png', dpi=300)
        plt.savefig(self.plots_dir / 'memory_vs_files.pdf')
        plt.close()
    
    def _plot_time_vs_filesize(self):
        """Plot execution time vs file size"""
        fig, ax = plt.subplots(figsize=(12, 6))
        
        for comp_alg in self.compression_algorithms:
            for enc_alg in self.encryption_algorithms:
                filtered = [r for r in self.results 
                           if r.compression_algorithm == comp_alg 
                           and r.encryption_algorithm == enc_alg]
                
                if not filtered:
                    continue
                
                x = [r.file_size / 1024 for r in filtered]  # KB
                y = [r.time_seconds for r in filtered]
                
                ax.plot(x, y, marker='s', label=f"{comp_alg}+{enc_alg}")
        
        ax.set_xlabel('File Size (KB)', fontsize=12)
        ax.set_ylabel('Execution Time (seconds)', fontsize=12)
        ax.set_title('Execution Time vs File Size', fontsize=14, fontweight='bold')
        ax.legend(loc='best', fontsize=9)
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'time_vs_filesize.png', dpi=300)
        plt.savefig(self.plots_dir / 'time_vs_filesize.pdf')
        plt.close()
    
    def _plot_compression_comparison(self):
        """Compare compression ratios across algorithms"""
        fig, ax = plt.subplots(figsize=(14, 7))
        
        # Calculate average compression ratio for each combination
        combinations = {}
        for result in self.results:
            key = f"{result.compression_algorithm}\n+{result.encryption_algorithm}"
            if key not in combinations:
                combinations[key] = []
            combinations[key].append(result.compression_ratio)
        
        labels = list(combinations.keys())
        values = [sum(v)/len(v) for v in combinations.values()]
        
        bars = ax.bar(range(len(labels)), values, color='steelblue', alpha=0.8)
        ax.set_xticks(range(len(labels)))
        ax.set_xticklabels(labels, fontsize=9)
        ax.set_ylabel('Compression Ratio (%)', fontsize=12)
        ax.set_title('Average Compression Ratio by Algorithm Combination', 
                     fontsize=14, fontweight='bold')
        ax.grid(True, axis='y', alpha=0.3)
        
        # Add value labels on bars
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{height:.1f}%', ha='center', va='bottom', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'compression_comparison.png', dpi=300)
        plt.savefig(self.plots_dir / 'compression_comparison.pdf')
        plt.close()
    
    def _plot_throughput_comparison(self):
        """Compare throughput across algorithms"""
        fig, ax = plt.subplots(figsize=(14, 7))
        
        combinations = {}
        for result in self.results:
            key = f"{result.compression_algorithm}\n+{result.encryption_algorithm}"
            if key not in combinations:
                combinations[key] = []
            combinations[key].append(result.throughput_mbps)
        
        labels = list(combinations.keys())
        values = [sum(v)/len(v) for v in combinations.values()]
        
        bars = ax.bar(range(len(labels)), values, color='darkorange', alpha=0.8)
        ax.set_xticks(range(len(labels)))
        ax.set_xticklabels(labels, fontsize=9)
        ax.set_ylabel('Throughput (MB/s)', fontsize=12)
        ax.set_title('Average Throughput by Algorithm Combination', 
                     fontsize=14, fontweight='bold')
        ax.grid(True, axis='y', alpha=0.3)
        
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{height:.2f}', ha='center', va='bottom', fontsize=9)
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'throughput_comparison.png', dpi=300)
        plt.savefig(self.plots_dir / 'throughput_comparison.pdf')
        plt.close()
    
    def _plot_resource_heatmap(self):
        """Create heatmap of resource efficiency"""
        import numpy as np
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
        
        # CPU heatmap
        cpu_matrix = np.zeros((len(self.compression_algorithms), 
                              len(self.encryption_algorithms)))
        
        for i, comp_alg in enumerate(self.compression_algorithms):
            for j, enc_alg in enumerate(self.encryption_algorithms):
                filtered = [r for r in self.results 
                           if r.compression_algorithm == comp_alg 
                           and r.encryption_algorithm == enc_alg]
                if filtered:
                    cpu_matrix[i, j] = sum(r.cpu_percent for r in filtered) / len(filtered)
        
        im1 = ax1.imshow(cpu_matrix, cmap='YlOrRd', aspect='auto')
        ax1.set_xticks(range(len(self.encryption_algorithms)))
        ax1.set_yticks(range(len(self.compression_algorithms)))
        ax1.set_xticklabels(self.encryption_algorithms)
        ax1.set_yticklabels(self.compression_algorithms)
        ax1.set_title('Average CPU Usage (%)', fontsize=12, fontweight='bold')
        
        # Add text annotations
        for i in range(len(self.compression_algorithms)):
            for j in range(len(self.encryption_algorithms)):
                ax1.text(j, i, f'{cpu_matrix[i, j]:.1f}',
                        ha="center", va="center", color="black", fontsize=10)
        
        plt.colorbar(im1, ax=ax1)
        
        # Memory heatmap
        mem_matrix = np.zeros((len(self.compression_algorithms), 
                              len(self.encryption_algorithms)))
        
        for i, comp_alg in enumerate(self.compression_algorithms):
            for j, enc_alg in enumerate(self.encryption_algorithms):
                filtered = [r for r in self.results 
                           if r.compression_algorithm == comp_alg 
                           and r.encryption_algorithm == enc_alg]
                if filtered:
                    mem_matrix[i, j] = sum(r.memory_mb for r in filtered) / len(filtered)
        
        im2 = ax2.imshow(mem_matrix, cmap='YlGnBu', aspect='auto')
        ax2.set_xticks(range(len(self.encryption_algorithms)))
        ax2.set_yticks(range(len(self.compression_algorithms)))
        ax2.set_xticklabels(self.encryption_algorithms)
        ax2.set_yticklabels(self.compression_algorithms)
        ax2.set_title('Average Memory Usage (MB)', fontsize=12, fontweight='bold')
        
        for i in range(len(self.compression_algorithms)):
            for j in range(len(self.encryption_algorithms)):
                ax2.text(j, i, f'{mem_matrix[i, j]:.1f}',
                        ha="center", va="center", color="black", fontsize=10)
        
        plt.colorbar(im2, ax=ax2)
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'resource_heatmap.png', dpi=300)
        plt.savefig(self.plots_dir / 'resource_heatmap.pdf')
        plt.close()
    
    def print_summary(self):
        """Print summary of benchmark results"""
        if not self.results:
            print("No results to summarize")
            return
        
        print(f"\n{'='*70}")
        print(f"  BENCHMARK SUMMARY")
        print(f"{'='*70}\n")
        
        # Overall statistics
        total_tests = len(self.results)
        successful = sum(1 for r in self.results if r.exit_code == 0)
        failed = total_tests - successful
        
        leaks = sum(1 for r in self.results if r.leaks_detected)
        zombies = sum(1 for r in self.results if r.zombies_detected)
        
        print(f"Total tests: {total_tests}")
        print(f"Successful: {successful} ({successful/total_tests*100:.1f}%)")
        print(f"Failed: {failed}")
        print(f"Memory leaks detected: {leaks}")
        print(f"Zombie processes detected: {zombies}")
        print()
        
        # Performance stats
        avg_time = sum(r.time_seconds for r in self.results) / total_tests
        avg_cpu = sum(r.cpu_percent for r in self.results) / total_tests
        avg_mem = sum(r.memory_mb for r in self.results) / total_tests
        avg_compression = sum(r.compression_ratio for r in self.results) / total_tests
        
        print(f"Average execution time: {avg_time:.2f} seconds")
        print(f"Average CPU usage: {avg_cpu:.1f}%")
        print(f"Average memory usage: {avg_mem:.1f} MB")
        print(f"Average compression ratio: {avg_compression:.1f}%")
        print()
        
        # Best performers
        best_time = min(self.results, key=lambda r: r.time_seconds)
        best_compression = max(self.results, key=lambda r: r.compression_ratio)
        best_throughput = max(self.results, key=lambda r: r.throughput_mbps)
        
        print("🏆 Best Performers:")
        print(f"  Fastest: {best_time.compression_algorithm}+{best_time.encryption_algorithm} "
              f"({best_time.time_seconds:.2f}s)")
        print(f"  Best compression: {best_compression.compression_algorithm}+"
              f"{best_compression.encryption_algorithm} ({best_compression.compression_ratio:.1f}%)")
        print(f"  Highest throughput: {best_throughput.compression_algorithm}+"
              f"{best_throughput.encryption_algorithm} ({best_throughput.throughput_mbps:.2f} MB/s)")
        
        print(f"\n{'='*70}\n")


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="GSEA Comprehensive Benchmark and Resource Testing Suite",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Quick test with small files
  python3 benchmark_tests.py --binary ./bin/gsea --quick

  # Full benchmark suite
  python3 benchmark_tests.py --binary ./bin/gsea --full

  # Custom configuration
  python3 benchmark_tests.py --binary ./bin/gsea --files 100 500 1000 --sizes 10240 102400

  # With valgrind (slow!)
  python3 benchmark_tests.py --binary ./bin/gsea --quick --valgrind
        """
    )
    
    parser.add_argument('--binary', '-b', default='./bin/gsea',
                       help='Path to GSEA binary (default: ./bin/gsea)')
    parser.add_argument('--output', '-o', default='benchmark_results',
                       help='Output directory for results (default: benchmark_results)')
    parser.add_argument('--quick', action='store_true',
                       help='Run quick test with minimal configurations')
    parser.add_argument('--full', action='store_true',
                       help='Run full benchmark suite (comprehensive)')
    parser.add_argument('--valgrind', action='store_true',
                       help='Enable valgrind memory leak detection (slow!)')
    parser.add_argument('--files', nargs='+', type=int,
                       help='Number of files to test (e.g., 10 50 100)')
    parser.add_argument('--sizes', nargs='+', type=int,
                       help='File sizes in bytes (e.g., 1024 10240 102400)')
    parser.add_argument('--pattern', choices=['random', 'text', 'repetitive'],
                       default='text', help='Data pattern for test files')
    
    args = parser.parse_args()
    
    # Define test configurations
    if args.quick:
        file_configs = [
            (10, 10 * 1024),      # 10 files × 10KB
            (50, 50 * 1024),      # 50 files × 50KB
        ]
    elif args.full:
        file_configs = [
            (10, 10 * 1024),      # 10 files × 10KB
            (50, 50 * 1024),      # 50 files × 50KB
            (100, 100 * 1024),    # 100 files × 100KB
            (500, 100 * 1024),    # 500 files × 100KB
            (100, 1024 * 1024),   # 100 files × 1MB
        ]
    elif args.files and args.sizes:
        file_configs = [(f, s) for f in args.files for s in args.sizes]
    else:
        # Default configuration
        file_configs = [
            (10, 10 * 1024),      # 10 files × 10KB
            (50, 50 * 1024),      # 50 files × 50KB
            (100, 100 * 1024),    # 100 files × 100KB
        ]
    
    try:
        # Create benchmark instance
        benchmark = GSEABenchmark(args.binary, args.output)
        
        # Run benchmarks
        benchmark.run_benchmarks(
            file_configs=file_configs,
            data_pattern=args.pattern,
            use_valgrind=args.valgrind
        )
        
        # Save results
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        benchmark.save_results_csv(f"results_{timestamp}.csv")
        
        # Generate visualizations
        benchmark.generate_plots()
        
        # Print summary
        benchmark.print_summary()
        
        print(f"✅ All results saved to: {benchmark.results_dir}/")
        
    except KeyboardInterrupt:
        print("\n\n⚠️  Benchmark interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
