#!/bin/bash

cd command_tests
./run_command_tests
./run_output_diff

cd ../harness_tests
./run_harness_tests
./run_output_diff

cd ../instruction_tests
./run_instruction_tests
./run_output_diff

cd ../compiled_tests
./run_compiled_tests
./run_output_diff