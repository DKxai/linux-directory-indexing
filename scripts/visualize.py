#!/usr/bin/env python3
"""
Visualization script for Directory Lookup Performance Benchmark.
Generates professional dark-themed charts comparing 4 indexing methods.
"""

import csv
import os
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

# ──── Professional Dark Color Palette ────
DARK_BG      = '#0D1117'
CARD_BG      = '#161B22'
GRID_COLOR   = '#30363D'
TEXT_COLOR   = '#E6EDF3'
TEXT_DIM     = '#8B949E'
ACCENT_CYAN  = '#58A6FF'

COLORS = {
    'Linear Search': '#F85149',   # Vibrant Red
    'Hash Table':    '#3FB950',   # Vibrant Green
    'B-Tree':        '#58A6FF',   # Vibrant Blue
    'HTree':         '#F0883E'    # Vibrant Orange
}

MARKERS = {
    'Linear Search': 'o',
    'Hash Table':    's',
    'B-Tree':        '^',
    'HTree':         'D'
}

LINE_LABELS = {
    'Linear Search': 'Linear Search  O(n)',
    'Hash Table':    'Hash Table  O(1)',
    'B-Tree':        'B-Tree  O(log n)',
    'HTree':         'HTree  O(log b+k)'
}

def load_data(csv_path):
    """Load benchmark CSV data into a structured dict."""
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
                    'n': [], 'insert_time_ms': [],
                    'lookup_hit_ns': [], 'lookup_miss_ns': [],
                    'comp_hit': [], 'comp_miss': [],
                    'delete_ns': [], 'delete_comp': [],
                    'memory_kb': []
                }
            d = data[method]
            d['n'].append(int(row['num_entries']))
            d['insert_time_ms'].append(float(row['insert_time_ns']) / 1e6)
            d['lookup_hit_ns'].append(float(row['avg_lookup_hit_ns']))
            d['lookup_miss_ns'].append(float(row['avg_lookup_miss_ns']))
            d['comp_hit'].append(float(row['avg_comparisons_hit']))
            d['comp_miss'].append(float(row['avg_comparisons_miss']))
            d['delete_ns'].append(float(row['avg_delete_time_ns']))
            d['delete_comp'].append(float(row['avg_delete_comparisons']))
            d['memory_kb'].append(float(row['memory_usage_bytes']) / 1024)

    return data


def set_dark_style():
    """Apply dark theme styling globally."""
    plt.rcParams.update({
        'figure.facecolor':    DARK_BG,
        'axes.facecolor':      CARD_BG,
        'axes.edgecolor':      GRID_COLOR,
        'axes.labelcolor':     TEXT_COLOR,
        'text.color':          TEXT_COLOR,
        'xtick.color':         TEXT_DIM,
        'ytick.color':         TEXT_DIM,
        'grid.color':          GRID_COLOR,
        'grid.alpha':          0.5,
        'grid.linestyle':      '--',
        'legend.facecolor':    CARD_BG,
        'legend.edgecolor':    GRID_COLOR,
        'legend.labelcolor':   TEXT_COLOR,
        'font.family':         'sans-serif',
        'font.size':           11,
        'axes.linewidth':      1.0,
        'lines.linewidth':     2.8,
        'lines.markersize':    9,
        'figure.dpi':          100,
        'savefig.dpi':         300,
        'savefig.facecolor':   DARK_BG,
        'savefig.bbox':        'tight',
        'savefig.pad_inches':  0.3,
    })


def style_axis(ax, title, xlabel, ylabel, logx=True, logy=True):
    """Apply consistent styling to an axis."""
    ax.set_title(title, fontsize=14, fontweight='bold', color=TEXT_COLOR, pad=12)
    ax.set_xlabel(xlabel, fontsize=11, fontweight='bold', color=TEXT_DIM)
    ax.set_ylabel(ylabel, fontsize=11, fontweight='bold', color=TEXT_DIM)
    if logx:
        ax.set_xscale('log')
    if logy:
        ax.set_yscale('log')
    ax.grid(True, which='both', ls='--', alpha=0.3)
    ax.tick_params(axis='both', which='both', length=4)
    # Format ticks nicely
    ax.xaxis.set_major_formatter(mticker.FuncFormatter(
        lambda x, _: f'{int(x):,}' if x >= 1 else f'{x}'
    ))


def add_glow(ax, x, y, color, alpha=0.08):
    """Add a subtle glow/fill under the line for depth."""
    ax.fill_between(x, y, alpha=alpha, color=color, linewidth=0)


def plot_line(ax, x, y, method, glow=True):
    """Plot a single method line with optional glow effect."""
    color = COLORS.get(method, '#FFF')
    marker = MARKERS.get(method, 'o')
    label = LINE_LABELS.get(method, method)
    ax.plot(x, y, label=label, color=color, marker=marker,
            linewidth=2.8, markersize=9, markeredgewidth=0.5,
            markeredgecolor='white', zorder=5)
    if glow:
        add_glow(ax, x, y, color)


def annotate_last_point(ax, x, y, text, color, offset=(15, 5)):
    """Annotate the last data point with a value label."""
    ax.annotate(text, xy=(x[-1], y[-1]),
                xytext=offset, textcoords='offset points',
                fontsize=8, fontweight='bold', color=color,
                arrowprops=dict(arrowstyle='->', color=color, lw=1.2),
                bbox=dict(boxstyle='round,pad=0.3', facecolor=CARD_BG,
                          edgecolor=color, alpha=0.9))


def plot_lookup_performance(data, output_dir):
    """Generate dual-panel lookup performance chart."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    fig.suptitle('Directory Lookup Performance Comparison',
                 fontsize=18, fontweight='bold', color=ACCENT_CYAN, y=0.98)

    # Lookup Hits
    for method, m in data.items():
        x, y = m['n'], m['lookup_hit_ns']
        plot_line(ax1, x, y, method)
    style_axis(ax1, 'Lookup Hit (Existing Entry)', 'Number of Entries (N)',
               'Average Lookup Time (ns)')
    ax1.legend(loc='upper left', fontsize=9, framealpha=0.9,
               borderpad=0.8, handlelength=2.5)
    # Annotate speedup
    for method in ['Hash Table', 'HTree', 'B-Tree']:
        if method in data:
            y = data[method]['lookup_hit_ns']
            x = data[method]['n']
            val = y[-1]
            if val < 1000:
                txt = f'{int(val):,} ns'
            else:
                txt = f'{val/1000:.1f} µs'
            annotate_last_point(ax1, x, y, txt, COLORS[method],
                                offset=(12, -15 if method == 'HTree' else 8))

    # Lookup Misses
    for method, m in data.items():
        plot_line(ax2, m['n'], m['lookup_miss_ns'], method)
    style_axis(ax2, 'Lookup Miss (Non-existent Entry)', 'Number of Entries (N)',
               'Average Lookup Time (ns)')
    ax2.legend(loc='upper left', fontsize=9, framealpha=0.9,
               borderpad=0.8, handlelength=2.5)

    plt.tight_layout()
    fig.subplots_adjust(top=0.90, wspace=0.25)
    plt.savefig(os.path.join(output_dir, 'lookup_performance.png'))
    plt.close()
    print("  ✓ Saved: lookup_performance.png")


def plot_delete_performance(data, output_dir):
    """Generate delete performance chart with annotations."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    fig.suptitle('Directory Deletion Performance Comparison',
                 fontsize=18, fontweight='bold', color=ACCENT_CYAN, y=0.98)

    # Delete Time
    for method, m in data.items():
        plot_line(ax1, m['n'], m['delete_ns'], method)
    style_axis(ax1, 'Average Deletion Time', 'Number of Entries (N)',
               'Average Delete Time (ns)')
    ax1.legend(loc='upper left', fontsize=9, framealpha=0.9)

    # Delete Comparisons
    for method, m in data.items():
        # Skip 0 values for log scale
        y = [max(v, 0.5) for v in m['delete_comp']]
        plot_line(ax2, m['n'], y, method, glow=False)
    style_axis(ax2, 'Average Delete Comparisons', 'Number of Entries (N)',
               'Number of Comparisons')
    ax2.legend(loc='upper left', fontsize=9, framealpha=0.9)

    plt.tight_layout()
    fig.subplots_adjust(top=0.90, wspace=0.25)
    plt.savefig(os.path.join(output_dir, 'delete_performance.png'))
    plt.close()
    print("  ✓ Saved: delete_performance.png")


def plot_insert_performance(data, output_dir):
    """Generate insert performance chart."""
    fig, ax = plt.subplots(figsize=(10, 7))
    fig.suptitle('Directory Insertion Performance Comparison',
                 fontsize=18, fontweight='bold', color=ACCENT_CYAN, y=0.98)

    for method, m in data.items():
        plot_line(ax, m['n'], m['insert_time_ms'], method)

    style_axis(ax, '', 'Number of Entries (N)',
               'Total Insertion Time (ms)')
    ax.legend(loc='upper left', fontsize=10, framealpha=0.9,
              borderpad=0.8, handlelength=2.5)

    # Add speedup annotation box
    if 'Linear Search' in data and 'B-Tree' in data:
        lin_val = data['Linear Search']['insert_time_ms'][-1]
        bt_val = data['B-Tree']['insert_time_ms'][-1]
        ht_val = data['HTree']['insert_time_ms'][-1] if 'HTree' in data else 0
        note = (f'@ 1M entries:\n'
                f'Linear: {lin_val:,.0f} ms\n'
                f'HTree:  {ht_val:,.0f} ms\n'
                f'B-Tree: {bt_val:,.0f} ms')
        ax.text(0.97, 0.05, note, transform=ax.transAxes,
                fontsize=9, verticalalignment='bottom',
                horizontalalignment='right',
                bbox=dict(boxstyle='round,pad=0.6', facecolor=CARD_BG,
                          edgecolor=GRID_COLOR, alpha=0.95),
                family='monospace', color=TEXT_DIM)

    plt.tight_layout()
    fig.subplots_adjust(top=0.92)
    plt.savefig(os.path.join(output_dir, 'insert_performance.png'))
    plt.close()
    print("  ✓ Saved: insert_performance.png")


def plot_memory_usage(data, output_dir):
    """Generate memory usage comparison chart."""
    fig, ax = plt.subplots(figsize=(10, 7))
    fig.suptitle('Memory Footprint Comparison',
                 fontsize=18, fontweight='bold', color=ACCENT_CYAN, y=0.98)

    for method, m in data.items():
        plot_line(ax, m['n'], m['memory_kb'], method)

    style_axis(ax, '', 'Number of Entries (N)',
               'Memory Usage (KB)')
    ax.legend(loc='upper left', fontsize=10, framealpha=0.9,
              borderpad=0.8, handlelength=2.5)

    # Add overhead annotation
    if 'Linear Search' in data and 'B-Tree' in data:
        lin = data['Linear Search']['memory_kb'][-1]
        ht_mem = data['HTree']['memory_kb'][-1] if 'HTree' in data else 0
        bt = data['B-Tree']['memory_kb'][-1]
        hash_mem = data['Hash Table']['memory_kb'][-1] if 'Hash Table' in data else 0
        note = (f'Overhead vs Linear @ 1M:\n'
                f'Hash:   {(hash_mem/lin - 1)*100:+.1f}%\n'
                f'HTree:  {(ht_mem/lin - 1)*100:+.1f}%\n'
                f'B-Tree: {(bt/lin - 1)*100:+.1f}%')
        ax.text(0.97, 0.05, note, transform=ax.transAxes,
                fontsize=9, verticalalignment='bottom',
                horizontalalignment='right',
                bbox=dict(boxstyle='round,pad=0.6', facecolor=CARD_BG,
                          edgecolor=GRID_COLOR, alpha=0.95),
                family='monospace', color=TEXT_DIM)

    plt.tight_layout()
    fig.subplots_adjust(top=0.92)
    plt.savefig(os.path.join(output_dir, 'memory_usage.png'))
    plt.close()
    print("  ✓ Saved: memory_usage.png")


def plot_summary_comparison(data, output_dir):
    """Generate a summary bar chart comparing all methods at 1M entries."""
    fig, axes = plt.subplots(1, 3, figsize=(18, 7))
    fig.suptitle('Performance Summary @ 1,000,000 Entries',
                 fontsize=18, fontweight='bold', color=ACCENT_CYAN, y=0.98)

    methods_order = ['Linear Search', 'Hash Table', 'B-Tree', 'HTree']
    methods = [m for m in methods_order if m in data]
    colors = [COLORS[m] for m in methods]
    bar_labels = ['Linear', 'Hash', 'B-Tree', 'HTree']
    bar_labels = [bar_labels[methods_order.index(m)] for m in methods]
    x = np.arange(len(methods))
    bar_width = 0.55

    # 1. Lookup Time (log scale)
    lookup_vals = [data[m]['lookup_hit_ns'][-1] for m in methods]
    bars = axes[0].bar(x, lookup_vals, bar_width, color=colors, edgecolor='white',
                       linewidth=0.5, alpha=0.9, zorder=5)
    axes[0].set_yscale('log')
    axes[0].set_title('Lookup Time (ns)', fontsize=13, fontweight='bold',
                      color=TEXT_COLOR, pad=10)
    axes[0].set_xticks(x)
    axes[0].set_xticklabels(bar_labels, fontweight='bold')
    axes[0].grid(axis='y', alpha=0.3, ls='--')
    # Add value labels on bars
    for bar, val in zip(bars, lookup_vals):
        if val < 1000:
            txt = f'{int(val):,}'
        elif val < 1e6:
            txt = f'{val/1000:.1f}K'
        else:
            txt = f'{val/1e6:.1f}M'
        axes[0].text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.3,
                     txt, ha='center', va='bottom', fontsize=9,
                     fontweight='bold', color=TEXT_COLOR)

    # 2. Delete Time (log scale)
    delete_vals = [data[m]['delete_ns'][-1] for m in methods]
    bars = axes[1].bar(x, delete_vals, bar_width, color=colors, edgecolor='white',
                       linewidth=0.5, alpha=0.9, zorder=5)
    axes[1].set_yscale('log')
    axes[1].set_title('Delete Time (ns)', fontsize=13, fontweight='bold',
                      color=TEXT_COLOR, pad=10)
    axes[1].set_xticks(x)
    axes[1].set_xticklabels(bar_labels, fontweight='bold')
    axes[1].grid(axis='y', alpha=0.3, ls='--')
    for bar, val in zip(bars, delete_vals):
        if val < 1000:
            txt = f'{int(val):,}'
        elif val < 1e6:
            txt = f'{val/1000:.1f}K'
        else:
            txt = f'{val/1e6:.1f}M'
        axes[1].text(bar.get_x() + bar.get_width()/2, bar.get_height() * 1.3,
                     txt, ha='center', va='bottom', fontsize=9,
                     fontweight='bold', color=TEXT_COLOR)

    # 3. Memory Usage
    mem_vals = [data[m]['memory_kb'][-1] / 1024 for m in methods]  # Convert to MB
    bars = axes[2].bar(x, mem_vals, bar_width, color=colors, edgecolor='white',
                       linewidth=0.5, alpha=0.9, zorder=5)
    axes[2].set_title('Memory Usage (MB)', fontsize=13, fontweight='bold',
                      color=TEXT_COLOR, pad=10)
    axes[2].set_xticks(x)
    axes[2].set_xticklabels(bar_labels, fontweight='bold')
    axes[2].grid(axis='y', alpha=0.3, ls='--')
    for bar, val in zip(bars, mem_vals):
        axes[2].text(bar.get_x() + bar.get_width()/2, bar.get_height() + 3,
                     f'{val:.0f} MB', ha='center', va='bottom', fontsize=9,
                     fontweight='bold', color=TEXT_COLOR)

    plt.tight_layout()
    fig.subplots_adjust(top=0.88, wspace=0.3)
    plt.savefig(os.path.join(output_dir, 'summary_comparison.png'))
    plt.close()
    print("  ✓ Saved: summary_comparison.png")


def plot_comparisons(data, output_dir):
    """Generate comparison count chart (lookup + delete)."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    fig.suptitle('Number of Comparisons Analysis',
                 fontsize=18, fontweight='bold', color=ACCENT_CYAN, y=0.98)

    # Lookup Comparisons
    for method, m in data.items():
        y = [max(v, 0.5) for v in m['comp_hit']]
        plot_line(ax1, m['n'], y, method, glow=False)
    style_axis(ax1, 'Lookup Comparisons (Hit)', 'Number of Entries (N)',
               'Number of Comparisons')
    ax1.legend(loc='upper left', fontsize=9, framealpha=0.9)

    # Annotate final values
    for method in data:
        y = data[method]['comp_hit']
        x = data[method]['n']
        val = y[-1]
        if val > 0:
            txt = f'{int(val):,}'
            annotate_last_point(ax1, x, [max(v, 0.5) for v in y],
                                txt, COLORS[method], offset=(12, 0))

    # Delete Comparisons
    for method, m in data.items():
        y = [max(v, 0.5) for v in m['delete_comp']]
        plot_line(ax2, m['n'], y, method, glow=False)
    style_axis(ax2, 'Delete Comparisons', 'Number of Entries (N)',
               'Number of Comparisons')
    ax2.legend(loc='upper left', fontsize=9, framealpha=0.9)

    plt.tight_layout()
    fig.subplots_adjust(top=0.90, wspace=0.25)
    plt.savefig(os.path.join(output_dir, 'comparisons.png'))
    plt.close()
    print("  ✓ Saved: comparisons.png")


def main():
    csv_path = 'results/benchmark_results.csv'
    output_dir = 'results'

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    print("\n  ╭─────────────────────────────────────────╮")
    print("  │  📊 Benchmark Visualization Generator   │")
    print("  ╰─────────────────────────────────────────╯\n")

    print("  Loading benchmark data...")
    data = load_data(csv_path)
    if not data:
        print("  ✗ Could not load data. Run './benchmark --full' first.")
        return

    methods = list(data.keys())
    sizes = data[methods[0]]['n']
    print(f"  Found {len(methods)} methods: {', '.join(methods)}")
    print(f"  Test sizes: {', '.join(str(s) for s in sizes)}")
    print(f"  Total test cases: {len(methods) * len(sizes)}\n")

    print("  Generating charts (dark theme)...")
    set_dark_style()
    plot_lookup_performance(data, output_dir)
    plot_delete_performance(data, output_dir)
    plot_insert_performance(data, output_dir)
    plot_memory_usage(data, output_dir)
    plot_summary_comparison(data, output_dir)
    plot_comparisons(data, output_dir)

    print(f"\n  ✓ All {6} charts saved to '{output_dir}/' folder!")
    print("  ✓ Resolution: 300 DPI (print-ready)\n")


if __name__ == '__main__':
    main()
