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
  %0 = load i64, i64* %counter
  %1 = load i64, i64* %number_of_repetitions
  %i_less_than = icmp slt i64 %0, %1
  br label %while_cond

while_cond:                                       ; preds = %while_block, %entry
  %2 = load i64, i64* %counter
  %while_cond1 = icmp ne i1 %i_less_than, false
  br i1 %while_cond1, label %while_block, label %while_aftr

while_block:                                      ; preds = %while_cond
  %add = add i64 %2, 1
  store i64 %add, i64* %counter
  br label %while_cond

while_aftr:                                       ; preds = %while_cond
  ret void
}
