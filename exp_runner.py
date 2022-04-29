import subprocess
import re
from typing import Callable, List, Optional
import tqdm
import csv


def run_exp_using_given_arg(arg: List[str], runtime_extractor: Callable[[str], Optional[int]], repetition_times: int, desc: str):
    result = []
    for _ in tqdm.tqdm(range(repetition_times), desc=desc):
        result.append(runtime_extractor(subprocess.run(arg, capture_output=True, text=True).stdout))
    return result


def extractor(s: str) -> Optional[int]:
    expression = r"(\d+) ns\.$"
    pattern = re.compile(expression)
    match_obj = pattern.search(s)
    if match_obj is not None:
        runtime = match_obj.group(1)
        return int(runtime)
    return None


input_filename = "bin/Debug/fake_id_short.wav"
ir_filename = "bin/Debug/IR_MEDIUM.wav"
output_filename = "bin/Debug/out.wav"
N_EXP = 100

with open("runtime.csv", "w", encoding="utf-8") as f:
    csv_writer = csv.writer(f)
    for block_size_bitshift in range(8, 17):
        csv_writer.writerow(
            [1 << block_size_bitshift] + 
            run_exp_using_given_arg(
                [
                    "bin/relwithdebinfo/MUSI6106Exec.exe",
                    "-t",
                    "freq",
                    "-g",
                    "0.1",
                    "-b",
                    str(1 << block_size_bitshift),
                    "-i",
                    input_filename,
                    "-r",
                    ir_filename,
                    "-o",
                    output_filename
                ],
                extractor,
                N_EXP,
                str(1 << block_size_bitshift)
            )
        )
    csv_writer.writerow(["time"] + run_exp_using_given_arg(
                [
                    "bin/relwithdebinfo/MUSI6106Exec.exe",
                    "-t",
                    "time",
                    "-g",
                    "0.1",
                    "-b",
                    "1024",
                    "-i",
                    input_filename,
                    "-r",
                    ir_filename,
                    "-o",
                    output_filename
                ],
                extractor,
                N_EXP,
                "time"
            ))
