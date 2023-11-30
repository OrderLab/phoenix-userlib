import csv

# Input and output file paths
input_file_path = 'phx-snap.csv'
output_file_path = 'phx.csv'

count = 0
# Open input and output files
with open(input_file_path, 'r') as infile, open(output_file_path, 'w', newline='') as outfile:
    reader = csv.reader(infile)
    writer = csv.writer(outfile)

    # Iterate through each row in the input CSV
    for row in reader:
        count = count + 1
        # Assuming you have at least two columns, subtract the second from the first
        if len(row) > 2:
            result = float(row[2]) - float(row[1])
            # Add the result as a new column in the output CSV
            writer.writerow([row[0]] + [result])
        else:
            if count % 2 == 1:
                writer.writerow(row)

print(f"Subtraction completed. Output saved to {output_file_path}")

