#!/usr/bin/env bash

make
chmod +x ./cucpp
EXIT_CODE=0
echo "Running tests in ./example..."
for test_file in ./example/*; do
    echo "Testing $test_file..."
    if ! ./cucpp "$test_file" 2>warnings.txt; then
        echo "Error: Compilation failed for $test_file"
        cat warnings.txt
        EXIT_CODE=1
    elif [ -s warnings.txt ]; then
        echo "Error: Warnings detected in $test_file (Banned!)"
        cat warnings.txt
        EXIT_CODE=1
    else
        echo "$test_file passed."
    fi
done

if (( EXIT_CODE != 0 )); then
    exit $EXIT_CODE
fi

cc -Wextra -Wall -Werror -g -I/run/current-system/sw/include -L/run/current-system/sw/lib -lOpenCL ./runtime/linux.c -o test_runtime
chmod +x ./test_runtime
./test_runtime CPU GPU >runtime_check;
if ! grep -q "^CPU memory allocated at:" runtime_check; then 
    EXIT_CODE=2
fi
if ! grep -q "^Using GPU device:" runtime_check; then 
    EXIT_CODE=2
fi
if ! grep -q "^GPU memory allocated at:" runtime_check; then 
    EXIT_CODE=2
fi

if (( EXIT_CODE == 0 )); then
    echo "Congrats all tests went through successfully!"
fi

exit $EXIT_CODE
