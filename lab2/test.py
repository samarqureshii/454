import csv
import random
import itertools

# Define possible actions and numeric ranges (positive & negative)
actions = ["W", "A", "S", "D", "MX", "MY", "CW", "CCW"]
values = list(range(-5, 6))  # includes -5 to +5 (skipping 0)

# Generate all possible combinations
all_combos = [(a, v) for a, v in itertools.product(actions, values) if v != 0]

# Randomize and repeat until we have 1000 entries
combos = []
while len(combos) < 1000:
    combos.extend(random.sample(all_combos, len(all_combos)))
combos = combos[:1000]

# Write to CSV
with open("action_combinations.csv", mode="w", newline="") as file:
    writer = csv.writer(file)
    for action, value in combos:
        writer.writerow([action, value])

print("✅ Generated action_combinations.csv with 1000 entries (including negatives).")
