#!/usr/bin/env python3
"""
Convert a Total Phase Data Center CSV log into a Markdown table.
Correctly handles:
- Commented header line starting with '# Level,Sp,Index,...'
- Data rows without headers
- 'Index' moved to first column
- Markdown-only output
- Safe, non-destructive behavior
- Optional filtering by string matches
"""
import argparse
import csv
import sys
from pathlib import Path

# Global filter set - if non-empty, only rows containing ANY of these strings will be included
# FILTER_STRINGS = set()
FILTER_STRINGS = {
    "Set Address",
    "Get Device Descriptor",
    "Get String Descriptor",
    "Set Configuration",
    "Get NTB",
    "Connection",
    "Set Interface",
    "NCM Transfer"
}

def parse_total_phase_csv(path: Path):
    header = None
    rows = []
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n\r")
            # Find the real header (commented)
            if line.startswith("#") and "Level,Sp,Index" in line:
                header = line.lstrip("# ").split(",")
                continue
            # Skip other comments / blank lines
            if not line or line.startswith("#"):
                continue
            # Data line
            row = next(csv.reader([line]))
            
            # Apply filter if FILTER_STRINGS is non-empty
            if FILTER_STRINGS:
                row_text = ",".join(row)  # Combine all cells for matching
                if not any(filter_str in row_text for filter_str in FILTER_STRINGS):
                    continue  # Skip this row
            
            rows.append(row)
    if header is None:
        raise RuntimeError("Could not find header line starting with '# Level,Sp,Index'")
    return header, rows

def write_markdown(header, rows, output_path: Path):
    if "Index" not in header:
        raise RuntimeError("Header does not contain 'Index' column")
    idx = header.index("Index")
    new_header = ["Index"] + [h for i, h in enumerate(header) if i != idx]
    with output_path.open("w", encoding="utf-8") as out:
        # Header
        out.write("| " + " | ".join(new_header) + " |\n")
        out.write("|" + "|".join(["---"] * len(new_header)) + "|\n")
        for row in rows:
            # Pad short rows
            if len(row) < len(header):
                row += [""] * (len(header) - len(row))
            reordered = [row[idx]] + [cell for i, cell in enumerate(row) if i != idx]
            escaped = [(cell or "").replace("|", "\\|") for cell in reordered]
            out.write("| " + " | ".join(escaped) + " |\n")

def main():
    ap = argparse.ArgumentParser(
        description="Convert Total Phase CSV log to Markdown (Index first)."
    )
    ap.add_argument("input", help="Total Phase CSV log file")
    ap.add_argument(
        "-o", "--output",
        help="Output Markdown file (default: <input_stem>.md)"
    )
    args = ap.parse_args()
    input_path = Path(args.input)
    if not input_path.exists():
        print(f"ERROR: Input file not found: {input_path}", file=sys.stderr)
        sys.exit(2)
    output_path = (
        Path(args.output)
        if args.output
        else input_path.with_suffix(".md")
    )
    if input_path.resolve() == output_path.resolve():
        print("ERROR: Refusing to overwrite input file.", file=sys.stderr)
        sys.exit(3)
    try:
        header, rows = parse_total_phase_csv(input_path)
        write_markdown(header, rows, output_path)
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(4)
    
    filter_msg = f" (filtered by: {FILTER_STRINGS})" if FILTER_STRINGS else ""
    print(f"Markdown written to: {output_path}{filter_msg}")
    print(f"Total rows included: {len(rows)}")

if __name__ == "__main__":
    main()