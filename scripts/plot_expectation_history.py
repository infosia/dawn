import sys
import random

def generate_svg(input_file, output_file):
    # 1. Read and Parse Data
    weeks = []
    dates = []
    totals = []
    # generations_data[week_index] = [count_gen0, count_gen1, ...]
    generations_data = [] 
    
    try:
        with open(input_file, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Error: File {input_file} not found.")
        return

    max_gen_count = 0
    
    for line in lines:
        line = line.strip()
        if not line or line.startswith('-'):
            continue
        
        parts = line.split()
        
        # Header check: "Week Date Total ..."
        if parts[0] == 'Week' and parts[1] == 'Date':
            continue
            
        # Data check: "Week 0 2025-..."
        if parts[0] == 'Week' and not parts[1].isdigit():
             # Just in case there's another header format
             continue
        
        if len(parts) < 5:
            continue
            
        week_num = int(parts[1])
        date_str = parts[2]
        total = int(parts[3])
        counts_str = parts[4]
        
        counts = [int(x) for x in counts_str.split(',')]
        
        weeks.append(week_num)
        dates.append(date_str)
        totals.append(total)
        generations_data.append(counts)
        max_gen_count = max(max_gen_count, len(counts))

    if not weeks:
        print("No data found.")
        return

    # Normalize data: ensure all weeks have entries for all generations (pad with 0)
    # Actually, for a stacked graph, we need the cumulative sums.
    # layers[gen_index][week_index] = cumulative_sum_up_to_gen_index
    
    num_weeks = len(weeks)
    # We need 'max_gen_count' layers.
    
    # Prepare layer data
    # layer_tops[gen_index][week_index] is the Y value of the top of this generation's band
    layer_tops = []
    
    for gen_idx in range(max_gen_count):
        week_tops = []
        for w_idx in range(num_weeks):
            # Sum counts for this week up to gen_idx
            counts = generations_data[w_idx]
            # If this generation doesn't exist yet for this week (shouldn't happen if logic is correct, but be safe), treat as 0
            # Actually, the counts list length grows.
            current_sum = sum(counts[i] for i in range(min(len(counts), gen_idx + 1)))
            week_tops.append(current_sum)
        layer_tops.append(week_tops)

    # 2. Setup SVG Dimensions
    width = 1000
    height = 600
    margin_left = 60
    margin_right = 20
    margin_top = 40
    margin_bottom = 60
    
    graph_width = width - margin_left - margin_right
    graph_height = height - margin_top - margin_bottom
    
    max_total = max(totals) if totals else 0
    if max_total == 0:
        max_total = 1 # Avoid div by zero
        
    # Scaling functions
    def get_x(w_idx):
        return margin_left + (w_idx / (num_weeks - 1)) * graph_width

    def get_y(val):
        # Y is inverted in SVG (0 is top)
        return margin_top + graph_height - (val / max_total) * graph_height

    # 3. Generate SVG Content
    svg_parts = []
    svg_parts.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
    svg_parts.append(f'<rect width="100%" height="100%" fill="white"/>')
    
    # Title
    svg_parts.append(f'<text x="{width/2}" y="{margin_top/2 + 5}" text-anchor="middle" font-family="sans-serif" font-size="16">Expectation History (Sedimentary Layers)</text>')

    # Colors
    # Generate a palette. We want distinct colors.
    # Simple approach: HSL rotation
    def get_color(idx, total_gens):
        hue = (idx * 137.508) % 360 # Golden angle approximation for distribution
        return f"hsl({hue}, 70%, 60%)"

    # Draw Layers (Back to Front or Front to Back? Order matters for overlap if we stroke, but we fill)
    # We draw from bottom up? No, we define paths.
    # Layer i is defined by points from layer_tops[i] (top edge) and layer_tops[i-1] (bottom edge).
    # For i=0, bottom edge is y=0.
    
    for gen_idx in range(max_gen_count):
        # Construct path
        path_d = []
        
        # Top edge (Left to Right)
        for w_idx in range(num_weeks):
            x = get_x(w_idx)
            y = get_y(layer_tops[gen_idx][w_idx])
            cmd = "M" if w_idx == 0 else "L"
            path_d.append(f"{cmd} {x:.1f} {y:.1f}")
            
        # Bottom edge (Right to Left)
        # If gen_idx == 0, bottom is 0 (which is get_y(0))
        for w_idx in range(num_weeks - 1, -1, -1):
            x = get_x(w_idx)
            if gen_idx == 0:
                y = get_y(0)
            else:
                y = get_y(layer_tops[gen_idx-1][w_idx])
            path_d.append(f"L {x:.1f} {y:.1f}")
            
        path_d.append("Z") # Close path
        
        color = get_color(gen_idx, max_gen_count)
        svg_parts.append(f'<path d="{" ".join(path_d)}" fill="{color}" stroke="none" opacity="0.9"><title>Gen {gen_idx}</title></path>')

    # Axes
    # Y Axis
    svg_parts.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{height - margin_bottom}" stroke="black"/>')
    # X Axis
    svg_parts.append(f'<line x1="{margin_left}" y1="{height - margin_bottom}" x2="{width - margin_right}" y2="{height - margin_bottom}" stroke="black"/>')
    
    # Y Axis Labels
    num_ticks = 10
    for i in range(num_ticks + 1):
        val = (max_total / num_ticks) * i
        y = get_y(val)
        svg_parts.append(f'<line x1="{margin_left-5}" y1="{y}" x2="{margin_left}" y2="{y}" stroke="black"/>')
        svg_parts.append(f'<text x="{margin_left-8}" y="{y+4}" text-anchor="end" font-family="sans-serif" font-size="10">{int(val)}</text>')
        
    # X Axis Labels (Dates)
    # Show maybe 5-10 dates
    step = max(1, num_weeks // 8)
    for w_idx in range(0, num_weeks, step):
        x = get_x(w_idx)
        date_str = dates[w_idx]
        svg_parts.append(f'<line x1="{x}" y1="{height - margin_bottom}" x2="{x}" y2="{height - margin_bottom + 5}" stroke="black"/>')
        svg_parts.append(f'<text x="{x}" y="{height - margin_bottom + 18}" text-anchor="middle" font-family="sans-serif" font-size="10" transform="rotate(0, {x}, {height - margin_bottom + 18})">{date_str}</text>')

    svg_parts.append('</svg>')
    
    with open(output_file, 'w') as f:
        f.write('\n'.join(svg_parts))
    
    print(f"Generated {output_file}")

if __name__ == '__main__':
    generate_svg('expectation_history.txt', 'expectation_history.svg')
