; ModuleID = 'mod_main'
source_filename = "mod_main"

define internal void @fn_main() {
entry:
  %b = alloca i64
  store i64 0, i64* %b
  store i1 false, i64* %b
  ret void
}
