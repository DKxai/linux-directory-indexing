#!/usr/bin/env python3
import csv
import os
import matplotlib.pyplot as plt

# Professional color palette (Glassmorphism-inspired, high-contrast)
COLORS = {
    'Linear Search': '#E06666',  # Soft Red
    'Hash Table': '#6AA84F',     # Soft Green
    'B-Tree': '#4A90E2',         # Soft Blue
    'HTree': '#E67E22'           # Soft Orange
}

MARKERS = {
    'Linear Search': 'o',
    'Hash Table': 's',
    'B-Tree': '^',
    'HTree': 'D'
}

def load_data(csv_path):
    if not os.path.exists(csv_path):
        print(f"Error: CSV file not found at {csv_path}")
        return None

    data = {}
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            method = row['method']
            if method not in data:
                data[method] = {
                    'n': [],
                    'insert_time_ms': [],
                    'lookup_hit_ns': [],
                    'lookup_miss_ns': [],
                    'comp_hit': [],
                    'comp_miss': [],
                    'delete_ns': [],
                    'delete_comp': [],
                    'memory_kb': []
                }
            
            data[method]['n'].append(int(row['num_entries']))
            data[method]['insert_time_ms'].append(float(row['insert_time_ns']) / 1e6) # ns to ms
            data[method]['lookup_hit_ns'].append(float(row['avg_lookup_hit_ns']))
            data[method]['lookup_miss_ns'].append(float(row['avg_lookup_miss_ns']))
            data[method]['comp_hit'].append(float(row['avg_comparisons_hit']))
            data[method]['comp_miss'].append(float(row['avg_comparisons_miss']))
            data[method]['delete_ns'].append(float(row['avg_delete_time_ns']))
            data[method]['delete_comp'].append(float(row['avg_delete_comparisons']))
            data[method]['memory_kb'].append(float(row['memory_usage_bytes']) / 1024) # bytes to KB

    return data

def set_style():
    plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
    plt.rcParams['font.family'] = 'sans-serif'
    plt.rcParams['font.size'] = 11
    plt.rcParams['axes.linewidth'] = 1.2
    plt.rcParams['grid.alpha'] = 0.4
    plt.rcParams['figure.facecolor'] = '#FDFDFD'
    plt.rcParams['axes.facecolor'] = '#F8F9FA'

def plot_lookup_performance(data, output_dir):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle('Directory Lookup Performance Comparison (Log-Log Scale)', fontsize=16, fontweight='bold', color='#2C3E50')

    # 1. Lookup Hits
    for method, metrics in data.items():
        ax1.plot(metrics['n'], metrics['lookup_hit_ns'], 
                 label=method, color=COLORS.get(method, '#333'), 
                 marker=MARKERS.get(method, 'o'), linewidth=2.5, markersize=8)
    
    ax1.set_xscale('log')
    ax1.set_yscale('log')
    ax1.set_title('Lookup Hit (Existing Entry)', fontsize=13, fontweight='bold', color='#34495E')
    ax1.set_xlabel('Number of Directory Entries (N)', fontweight='bold')
    ax1.set_ylabel('Average Lookup Time (nanoseconds)', fontweight='bold')
    ax1.grid(True, which="both", ls="--")
    ax1.legend(frameon=True, facecolor='white', framealpha=0.9)

    # 2. Lookup Misses
    for method, metrics in data.items():
        ax2.plot(metrics['n'], metrics['lookup_miss_ns'], 
                 label=method, color=COLORS.get(method, '#333'), 
                 marker=MARKERS.get(method, 'o'), linewidth=2.5, markersize=8)
    
    ax2.set_xscale('log')
    ax2.set_yscale('log')
    ax2.set_title('Lookup Miss (Non-existent Entry)', fontsize=13, fontweight='bold', color='#34495E')
    ax2.set_xlabel('Number of Directory Entries (N)', fontweight='bold')
    ax2.set_ylabel('Average Lookup Time (nanoseconds)', fontweight='bold')
    ax2.grid(True, which="both", ls="--")
    ax2.legend(frameon=True, facecolor='white', framealpha=0.9)

    plt.tight_layout()
    fig.subplots_adjust(top=0.88)
    plt.savefig(os.path.join(output_dir, 'lookup_performance.png'), dpi=300)
    plt.close()
    print("  ✓ Saved: lookup_performance.png")

def plot_delete_performance(data, output_dir):
    plt.figure(figsize=(9, 6))
    plt.title('Directory Deletion Performance Comparison', fontsize=14, fontweight='bold', color='#2C3E50')

    for method, metrics in data.items():
        plt.plot(metrics['n'], metrics['delete_ns'], 
                 label=method, color=COLORS.get(method, '#333'), 
                 marker=MARKERS.get(method, 'o'), linewidth=2.5, markersize=8)

    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Directory Entries (N)', fontweight='bold')
    plt.ylabel('Average Deletion Time (nanoseconds)', fontweight='bold')
    plt.grid(True, which="both", ls="--")
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'delete_performance.png'), dpi=300)
    plt.close()
    print("  ✓ Saved: delete_performance.png")

def plot_insert_performance(data, output_dir):
    plt.figure(figsize=(9, 6))
    plt.title('Directory Insertion Performance Comparison', fontsize=14, fontweight='bold', color='#2C3E50')

    for method, metrics in data.items():
        plt.plot(metrics['n'], metrics['insert_time_ms'], 
                 label=method, color=COLORS.get(method, '#333'), 
                 marker=MARKERS.get(method, 'o'), linewidth=2.5, markersize=8)

    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Directory Entries (N)', fontweight='bold')
    plt.ylabel('Total Insertion Time (milliseconds)', fontweight='bold')
    plt.grid(True, which="both", ls="--")
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'insert_performance.png'), dpi=300)
    plt.close()
    print("  ✓ Saved: insert_performance.png")

def plot_memory_usage(data, output_dir):
    plt.figure(figsize=(9, 6))
    plt.title('Memory Footprint Comparison', fontsize=14, fontweight='bold', color='#2C3E50')

    for method, metrics in data.items():
        plt.plot(metrics['n'], metrics['memory_kb'], 
                 label=method, color=COLORS.get(method, '#333'), 
                 marker=MARKERS.get(method, 'o'), linewidth=2.5, markersize=8)

    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Number of Directory Entries (N)', fontweight='bold')
    plt.ylabel('Memory Usage (KB)', fontweight='bold')
    plt.grid(True, which="both", ls="--")
    plt.legend(frameon=True, facecolor='white', framealpha=0.9)
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'memory_usage.png'), dpi=300)
    plt.close()
    print("  ✓ Saved: memory_usage.png")

def main():
    csv_path = 'results/benchmark_results.csv'
    output_dir = 'results'

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    print("Loading benchmark data...")
    data = load_data(csv_path)
    if not data:
        print("Error: Could not load data. Run './benchmark --full' first to generate results.")
        return

    print("Generating plots...")
    set_style()
    plot_lookup_performance(data, output_dir)
    plot_delete_performance(data, output_dir)
    plot_insert_performance(data, output_dir)
    plot_memory_usage(data, output_dir)
    print("\nAll visualization charts have been successfully generated in the 'results/' folder! 🚀")

if __name__ == '__main__':
    main()
