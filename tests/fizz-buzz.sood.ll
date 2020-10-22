; ModuleID = 'mod_main'
source_filename = "mod_main"

@numeric_fmt_spc = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@string_fmt_spc = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@l_str = private unnamed_addr constant [9 x i8] c"FizzBuzz\00", align 1
@l_str.1 = private unnamed_addr constant [5 x i8] c"Fizz\00", align 1
@l_str.2 = private unnamed_addr constant [5 x i8] c"Buzz\00", align 1
@l_str.3 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@l_str.4 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@l_str.5 = private unnamed_addr constant [3 x i8] c": \00", align 1
@l_str.6 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1

declare i32 @printf(i8*, ...)

define internal void @main() {
entry:
  %number_of_repetitions = alloca i64
  store i64 15, i64* %number_of_repetitions
  %output = alloca i8*
  store i8* getelementptr inbounds ([1 x i8], [1 x i8]* @l_str.4, i32 0, i32 0), i8** %output
  %counter = alloca i64
  store i64 0, i64* %counter
  br label %until_cond

until_cond:                                       ; preds = %until_block, %entry
  %_val_load = load i64, i64* %counter
  %_val_load1 = load i64, i64* %number_of_repetitions
  %add = add i64 %_val_load1, 1
  %i_equal = icmp eq i64 %_val_load, %add
  %until_cond2 = icmp ne i1 %i_equal, true
  br i1 %until_cond2, label %until_block, label %until_aftr

until_block:                                      ; preds = %until_cond
  %_val_load3 = load i64, i64* %counter
  %_f_call = call i8* @fizz_buzz(i64 %_val_load3)
  store i8* %_f_call, i8** %output
  %_val_load4 = load i64, i64* %counter
  %_printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @numeric_fmt_spc, i32 0, i32 0), i64 %_val_load4)
  %_printf_call5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @string_fmt_spc, i32 0, i32 0), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @l_str.5, i32 0, i32 0))
  %_val_load6 = load i8*, i8** %output
  %_printf_call7 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @string_fmt_spc, i32 0, i32 0), i8* %_val_load6)
  %_printf_call8 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @string_fmt_spc, i32 0, i32 0), i8* getelementptr inbounds ([2 x i8], [2 x i8]* @l_str.6, i32 0, i32 0))
  %_val_load9 = load i64, i64* %counter
  %add10 = add i64 %_val_load9, 1
  store i64 %add10, i64* %counter
  br label %until_cond

until_aftr:                                       ; preds = %until_cond
  ret void
}

define internal i8* @fizz_buzz(i64 %num) {
fizz_buzz__entry:
  %num1 = alloca i64
  store i64 %num, i64* %num1
  %_val_load = load i64, i64* %num1
  %srem_mod = srem i64 %_val_load, 3
  %i_equal = icmp eq i64 %srem_mod, 0
  %_val_load2 = load i64, i64* %num1
  %srem_mod3 = srem i64 %_val_load2, 5
  %i_equal4 = icmp eq i64 %srem_mod3, 0
  %also = and i1 %i_equal, %i_equal4
  %if_cond = icmp ne i1 %also, false
  br i1 %if_cond, label %if_then, label %if_else

if_then:                                          ; preds = %fizz_buzz__entry
  ret i8* getelementptr inbounds ([9 x i8], [9 x i8]* @l_str, i32 0, i32 0)
  br label %if_cnt

if_else:                                          ; preds = %fizz_buzz__entry
  br label %if_cnt

if_cnt:                                           ; preds = %if_else, %if_then
  %_val_load5 = load i64, i64* %num1
  %srem_mod6 = srem i64 %_val_load5, 3
  %i_equal7 = icmp eq i64 %srem_mod6, 0
  %if_cond8 = icmp ne i1 %i_equal7, false
  br i1 %if_cond8, label %if_then9, label %if_else10

if_then9:                                         ; preds = %if_cnt
  ret i8* getelementptr inbounds ([5 x i8], [5 x i8]* @l_str.1, i32 0, i32 0)
  br label %if_cnt11

if_else10:                                        ; preds = %if_cnt
  br label %if_cnt11

if_cnt11:                                         ; preds = %if_else10, %if_then9
  %_val_load12 = load i64, i64* %num1
  %srem_mod13 = srem i64 %_val_load12, 5
  %i_equal14 = icmp eq i64 %srem_mod13, 0
  %if_cond15 = icmp ne i1 %i_equal14, false
  br i1 %if_cond15, label %if_then16, label %if_else17

if_then16:                                        ; preds = %if_cnt11
  ret i8* getelementptr inbounds ([5 x i8], [5 x i8]* @l_str.2, i32 0, i32 0)
  br label %if_cnt18

if_else17:                                        ; preds = %if_cnt11
  br label %if_cnt18

if_cnt18:                                         ; preds = %if_else17, %if_then16
  ret i8* getelementptr inbounds ([1 x i8], [1 x i8]* @l_str.3, i32 0, i32 0)
}
