import pandas as pd
import os
import argparse

def load_csv(file_path):
    """Load a CSV file into a DataFrame."""
    return pd.read_csv(file_path)

def extract_prefix_and_column(column_name):
    """Extract prefix and column name from the format <prefix>_columnname."""
    if '_' in column_name:
        prefix, col = column_name.split(':', 1)
        return prefix, col
    return None, column_name

def main():
    parser = argparse.ArgumentParser(description="Select columns based on file prefixes and merge them.")
    parser.add_argument('files', nargs='+', help='Paths to CSV files')
    parser.add_argument('-c', '--columns', nargs='+', required=True,
                        help='List of columns to select with prefixes (e.g., file1_col1, file2_col2)')
    parser.add_argument('-o', '--output', required=True, help='Output CSV file path')
    
    args = parser.parse_args()

    # Load all CSV files into a dictionary with the base filename as the key
    dataframes = {}
    for file in args.files:
        base_filename = os.path.splitext(os.path.basename(file))[0]
        dataframes[base_filename] = load_csv(file)
        print(f'Loading {file}')
    
    # Prepare an empty DataFrame to store selected columns
    merged_df = pd.DataFrame()

    # Iterate over the specified columns
    for col_name in args.columns:
        prefix, col = extract_prefix_and_column(col_name)
        
        print(f'Processing {prefix}:{col}')
        # Check if the prefix matches any loaded file
        if prefix in dataframes:
            df = dataframes[prefix]
            print(f'Got data frame {prefix} with cols {df.columns.tolist()}')
            
            # Ensure the column exists in the corresponding DataFrame
            if col in df.columns:
                merged_df[f"{prefix}_{col}"] = df[col]
            else:
                print(f"Warning: Column '{col}' not found in file '{prefix}'. Skipping...")
        else:
            print(f"Warning: No CSV file found with prefix '{prefix}'. Skipping...")

    # Write the merged DataFrame to the output CSV file
    merged_df.to_csv(args.output, index=False)
    print(f"Data successfully written to {args.output}")

if __name__ == "__main__":
    main()
