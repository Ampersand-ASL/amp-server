filename = "capture.txt"

buckets = { }
low_bucket = 0
high_bucket = 0
first = True 

with open(filename, 'r') as file:
    for line in file:
        tokens = line.strip().split(",")
        if len(tokens) < 2:
            continue
        clean_tokens = []
        for token in tokens:
            clean_tokens.append(token.strip())
        # Make timestamp a number
        clean_tokens[0] = int(clean_tokens[0])
        ms = clean_tokens[0] / 1000
        bucket = int(ms / 4)
        bucket = bucket * 4000
        if not bucket in buckets:
            buckets[bucket] = [ ]
        buckets[bucket].append(clean_tokens)
        if first or low_bucket > bucket:
            low_bucket = bucket
        if high_bucket < bucket:
            high_bucket = bucket 
        first = False 

sorted_buckets = {}
# Sort all of the buckets chronologically
for key, value in buckets.items():
    sorted_buckets[key] = sorted(value, key=lambda p: p[0])

#print(low_bucket, high_bucket)
#print(sorted_buckets)

# Iterate across the time period and create display
step = 5000
last_origin_ms = 0
for bucket in range(low_bucket, high_bucket + step, step):
    # Build a list of what was received 
    rx_list = []
    sequence_fault = False
    if bucket in sorted_buckets:            
        for event in sorted_buckets[bucket]:
            if event[1] == "RXV":
                origin_ms = int(event[2])
                rx_list.append(origin_ms)
                # Look for ordering problems/gaps
                if last_origin_ms != 0 and (origin_ms < last_origin_ms or origin_ms > last_origin_ms + 25):
                    sequence_fault = True
                last_origin_ms = origin_ms
    print(bucket, rx_list, sequence_fault)
