from collections import OrderedDict

# CH340 baudrates
TARGET_BAUDRATES = [
    #50, 75,
    #100, 110, 134.5, 150, 300, 600, 900,
    1200, 1800, 2400, 3600, 4800, 9600,
    14400, 19200, 28800, 33600, 38400, 56000, 57600, 76800,
    115200, 128000, 153600, 230400, 460800, 921600,
    1500000, 2000000
]

# Arduino Monitor baudrates
TARGET_BAUDRATES = [
    300,
    1200, 2400, 4800, 9600,
    19200, 38400, 57600, 74880, 
    115200, 230400, 250000, 500000,
    1000000, 2000000
]

CPU_CLOCK = 16 * 1000000
OVERHEAD_CYCLES = 8
MAX_DEVIATION = 0.2 / 100

def find_delays():
    results = OrderedDict()
    delay_cycles = 0
    while True:
        actual_baudrate = (CPU_CLOCK / (delay_cycles + OVERHEAD_CYCLES))

        for i, target_baudrate in enumerate(sorted(TARGET_BAUDRATES)):
            deviation = (target_baudrate - actual_baudrate) / target_baudrate
            if abs(deviation) <= MAX_DEVIATION:
                if target_baudrate not in results:
                    results[target_baudrate] = []
                results[target_baudrate].append((delay_cycles, deviation))
        
        if actual_baudrate < min(TARGET_BAUDRATES) - min(TARGET_BAUDRATES) * MAX_DEVIATION:
            break

        delay_cycles += 1
    return results

if __name__ == "__main__":
    results = find_delays()

    print("Baud\tCycles\tDeviation")
    for target_baudrate, options in results.items():
        options.sort(key=lambda i: i[1])
        delay_cycles, deviation = options[0]
        print("{}\t{}\t{:.5f}%".format(target_baudrate, delay_cycles, deviation * 100))
