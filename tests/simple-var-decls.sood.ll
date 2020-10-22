; ModuleID = 'mod_main'
source_filename = "mod_main"

@numeric_fmt_spc = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@string_fmt_spc = private unnamed_addr constant [3 x i8] c"%s\00", align 1

declare i32 @printf(i8*, ...)

define internal void @main() {
entry:
  %b = alloca i64
  store i64 0, i64* %b
  store i64 2, i64* %b
  %_val_load = load i64, i64* %b
  %_printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @numeric_fmt_spc, i32 0, i32 0), i64 %_val_load)
  ret void
}
