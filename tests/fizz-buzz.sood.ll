; ModuleID = 'mod_main'
source_filename = "mod_main"

@numeric_fmt_spc = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@string_fmt_spc = private unnamed_addr constant [3 x i8] c"%s\00", align 1

declare i32 @printf(i8*, ...)

define internal void @main() {
entry:
  %number_of_repetitions = alloca i64
  store i64 10, i64* %number_of_repetitions
  %counter = alloca i64
  store i64 0, i64* %counter
  br label %while_cond

while_cond:                                       ; preds = %entry
  %0 = load i64, i64* %counter
  %1 = load i64, i64* %number_of_repetitions
  %i_less_than = icmp slt i64 %0, %1
  %while_cond1 = icmp ne i1 %i_less_than, false
  br i1 %while_cond1, label %while_block, label %while_aftr

while_block:                                      ; preds = %while_cond
  %2 = load i64, i64* %counter
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @numeric_fmt_spc, i32 0, i32 0), i64 %2)
  %4 = load i64, i64* %counter
  %add = add i64 %4, 1
  store i64 %add, i64* %counter
  ret void

while_aftr:                                       ; preds = %while_cond
}
