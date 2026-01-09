filename = "ecr_qso_1.txt"

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
        bucket = int(ms / 5)
        bucket = bucket * 5000
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

#quit()

# Iterate across the time period and create display
step = 5000
last_origin_ms = 0
first_delta = 0

# 1767975601956000

for bucket in range(low_bucket, high_bucket + step, step):
    # Build a list of what was received 
    rx_list = []
    sequence_fault = False
    delta = 0
    unkey = False
    played = None
    interpolate = False
    if bucket in sorted_buckets:            
        for event in sorted_buckets[bucket]:
            if event[1] == "RXV":
                origin_ms = int(event[2])
                rx_list.append(origin_ms)
                # Look for origin ordering problems/gaps
                if last_origin_ms != 0 and (origin_ms < last_origin_ms or origin_ms > last_origin_ms + 25):
                    sequence_fault = True
                last_origin_ms = origin_ms
                if first_delta == 0:
                    first_delta = event[0] - origin_ms * 1000
                delta = first_delta - (event[0] - origin_ms * 1000)
            elif event[1] == "UNK":
                unkey = True
            elif event[1] == "POV":
                played = event[2]
            elif event[1] == "POI":
                interpolate = True
    rx_list_str = ""
    for rx in rx_list:
        rx_list_str += " " + str(rx)
    sequence_fault_str = ""
    if sequence_fault:
        sequence_fault_str = "*"
    unkey_str = ""
    if unkey:
        unkey_str = "U"
    delta_str = ""
    if delta:
        delta_str = str(int(delta / 1000))
    played_str = ""
    if played:
        played_str = str(played)
    if interpolate:
        played_str = "I"
    print(bucket, "\t", rx_list_str, "\t", sequence_fault_str, "\t", unkey_str, "\t", delta_str, "\t", played_str)
    if bucket % 20000 == 0:
        print("-----------------------------------------------------------------------")
