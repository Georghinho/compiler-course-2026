; RUN: opt -load-pass-plugin %llvmshlibdir/example_LLVM_IR%pluginext\
; RUN: -passes=example -S %s | FileCheck %s

; Floating-point remainder
; CHECK-LABEL: define dso_local double @fp_rem(double %a, double %b)
; CHECK-NOT: frem
; CHECK: %[[DIV:.+]] = fdiv double %a, %b
; CHECK: %[[MUL:.+]] = fmul double %[[DIV]], %b
; CHECK: %[[SUB:.+]] = fsub double %a, %[[MUL]]
; CHECK: ret double %[[SUB]]
define dso_local double @fp_rem(double %a, double %b) {
entry:
  %r = frem double %a, %b
  ret double %r
}

; Signed integer remainder
; CHECK-LABEL: define dso_local i32 @srem_func(i32 %a, i32 %b)
; CHECK-NOT: srem
; CHECK: %[[SDIV:.+]] = sdiv i32 %a, %b
; CHECK: %[[MUL:.+]] = mul i32 %[[SDIV]], %b
; CHECK: %[[SUB:.+]] = sub i32 %a, %[[MUL]]
; CHECK: ret i32 %[[SUB]]
define dso_local i32 @srem_func(i32 %a, i32 %b) {
entry:
  %r = srem i32 %a, %b
  ret i32 %r
}

; Unsigned integer remainder
; CHECK-LABEL: define dso_local i32 @urem_func(i32 %a, i32 %b)
; CHECK-NOT: urem
; CHECK: %[[UDIV:.+]] = udiv i32 %a, %b
; CHECK: %[[MUL:.+]] = mul i32 %[[UDIV]], %b
; CHECK: %[[SUB:.+]] = sub i32 %a, %[[MUL]]
; CHECK: ret i32 %[[SUB]]
define dso_local i32 @urem_func(i32 %a, i32 %b) {
entry:
  %r = urem i32 %a, %b
  ret i32 %r
}