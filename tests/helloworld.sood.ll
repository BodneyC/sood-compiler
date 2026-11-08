; ModuleID = 'mod_main'
source_filename = "mod_main"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin27.2.0"

@str = private unnamed_addr constant [13 x i8] c"hello world\0A\00", align 1
@str.1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1

define i32 @main() {
entry:
  %write = call i32 (i32, ptr, ...) @dprintf(i32 1, ptr @str.1, ptr @str)
  ret i32 0
}

declare i32 @dprintf(i32, ptr, ...)
