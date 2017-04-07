; ModuleID = '<stdin>'
source_filename = "test1.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.12.0"

; Function Attrs: nounwind ssp uwtable
define i32 @main() #0 {
  %1 = alloca i32
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 0, i32* %2, align 4
  store i32 1, i32* %3, align 4
  store i32 2, i32* %4, align 4
  %7 = load i32, i32* %3, align 4
  %8 = icmp ne i32 %7, 0
  %9 = load i32, i32* %3
  %10 = load i32, i32* %4
  %11 = add i32 %9, %10
  store i32 %11, i32* %1
  br i1 %8, label %12, label %17

; <label>:12:                                     ; preds = %0
  %13 = load i32, i32* %3
  %14 = load i32, i32* %4
  %15 = add i32 %13, %14
  store i32 %15, i32* %1
  %16 = load i32, i32* %1
  store i32 %16, i32* %5, align 4
  br label %17

; <label>:17:                                     ; preds = %12, %0
  %18 = load i32, i32* %1
  store i32 %18, i32* %6, align 4
  %19 = load i32, i32* %6, align 4
  ret i32 %19
}

attributes #0 = { nounwind ssp uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+sse4.1,+ssse3" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"Apple LLVM version 8.0.0 (clang-800.0.42.1)"}
