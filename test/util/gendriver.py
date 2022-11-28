#!/usr/bin/env python3

import io
import sys

import glob
import re
import argparse

#find all the tests in the test files
# TODO: make sure that TEST() regex matches aren't commented out
def discover_tests(file_glob="test*.c", exclude_files=[], \
                   search_str="^TEST\(.*\)"):
  test_file_list = list(set(glob.glob(file_glob)) - set(exclude_files))
  test_file_list.sort()
  test_expr = re.compile(search_str, re.M)
  test_names = []
  for file in test_file_list:
    with open(file, "r") as f:
      file_str = f.read()
    new_matches = test_expr.findall(file_str)
    # extract test_name from the regex match 'TEST(test_name)'
    test_names += [s.split('(')[1].split(')')[0] for s in new_matches]
  return test_names

def gen_test_driver(tests, in_file="test_main.c.in", out_file=None):
  if out_file == None:
    out_file = in_file.split(".in")[0]
  test_declarations = "\n".join(["TEST(" + t + ");" for t in tests])
  n_tests = str(len(tests))
  test_list = "{" + ', '.join(["test_" + t for t in tests]) + "}"
  test_names = "{" + ', '.join(['"' + t + '"' for t in tests]) + "}"
  driver_file_buf = io.StringIO()
  driver_file_buf.write("// This file was generated ")
  driver_file_buf.write("automatically with util/gendriver.py\n\n")
  with open(in_file, "r") as f:
    in_file_str = f.read()
  out_file_str = in_file_str \
                  .replace("%%TEST_DECLARATIONS%%", test_declarations) \
                  .replace("%%N_TESTS%%", n_tests) \
                  .replace("%%TEST_LIST%%", test_list) \
                  .replace("%%TEST_NAMES%%", test_names)
  
  driver_file_buf.write("\n")
  driver_file_buf.write(out_file_str)
  driver_file_buf.seek(0)
  with open(out_file, "w") as f:
    f.write(driver_file_buf.read())

# TODO: command-line arguments
if __name__ == "__main__":
  parser = argparse.ArgumentParser(sys.argv[1:])
  parser.add_argument('--exclude_files', nargs='*', default=list())
  parsed_args = parser.parse_args()
  test_list = discover_tests(exclude_files=parsed_args.exclude_files)
  gen_test_driver(test_list)
