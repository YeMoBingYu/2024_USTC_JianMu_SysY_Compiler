#!/bin/bash

if [[ $# -ne 3  ]]; then
    echo "usage: $0 <compile_timeout> <execute_timeout> <stack_limit>"
    exit
fi

compile_timeout=$1
execute_timeout=$2
ulimit -s $3

run_case() {
    testcase_dir=$1
    testcase_name=$(basename "$2" .sy)
    out_file=output.txt

    # TODO: 修改这里的编译命令，请注意这里的相对路径，编译器必须在 ../build 目录下，助教不会使用 make install 安装你的编译器，这段代码最后应该能够编译出可执行程序 $testcase_name
    # -----修改开始-----
    timeout --foreground "$compile_timeout" ../build/SysY -S -o2 "$2" -o "$testcase_name".s
    as "$testcase_name".s -o "$testcase_name".o
    ld "$testcase_name".o /root/libsylib.a /usr/lib64/crt1.o /usr/lib64/crtn.o /usr/lib64/crti.o -lc -o "$testcase_name"
    # -----修改停止-----

    if [ -f "$testcase_dir/$testcase_name.in" ]; then
        timeout --foreground "$execute_timeout" ./"$testcase_name" <"$testcase_dir/$testcase_name.in" >output.txt 2>$testcase_name.stderr
    else
        timeout --foreground "$execute_timeout" ./"$testcase_name" >$out_file 2>$testcase_name.stderr
    fi
    ret=$?
    sed -i "\$s/\x00*$//" "$out_file"
    if [ -s "$out_file" ]; then
        # Check if the last character of the output file is not a newline
        if [ "$(tail -c1 "$out_file" | wc -l)" -eq 0 ]; then
            echo "" >>"$out_file"
        fi
    fi
    echo $ret >>"$out_file"

    time=$(tail -n 1 $testcase_name.stderr)
    echo -n "$2: "
    if cmp -s "$out_file" "$testcase_dir/$testcase_name".out; then
        ((pass += 1))
        echo "passed, $time"
    else
        time="TOTAL: 999H-999M-999S-999us"
        echo "fail"
    fi
    echo "$2: $time" >> $log_name


    rm -f "$testcase_name".s "$testcase_name".o "$testcase_name" $out_file $testcase_name.stderr
}

run_test(){
    pass=0
    all=0
    parentdir=$(dirname "$(pwd)")
    log_name=$(basename "$parentdir")_performance_score_$1.txt
    truncate -s 0 $log_name
    for testcase in testcases/performance/*.sy; do
        ((all += 1))
        run_case "testcases/performance" "$testcase"
    done

    for testcase in testcases/final_performance/*.sy; do
        ((all += 1))
        run_case "testcases/final_performance" "$testcase"
    done

    sed -i "1iPerformance test: $pass/$all passed" "$log_name"
}

for (( i=1; i<=5; i++ ))
do
    echo "== testing in $(dirname $(pwd)) (time limit of compile/execute: $compile_timeout/$execute_timeout), iter $i"
    run_test $i
done
