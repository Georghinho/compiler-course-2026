; RUN: opt -load-pass-plugin %llvmshlibdir/example_LLVM_IR%pluginext \
; RUN: -passes=example -S %s | FileCheck %s

; ============================================================
; Basic FP and integer cases (existing coverage extended)
; ============================================================

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

; ============================================================
; Vector types
; ============================================================

; CHECK-LABEL: define dso_local <2 x double> @v2f64_rem(<2 x double> %a, <2 x double> %b)
; CHECK-NOT: frem
; CHECK: %[[DIV:.+]] = fdiv <2 x double> %a, %b
; CHECK: %[[MUL:.+]] = fmul <2 x double> %[[DIV]], %b
; CHECK: %[[SUB:.+]] = fsub <2 x double> %a, %[[MUL]]
; CHECK: ret <2 x double> %[[SUB]]
define dso_local <2 x double> @v2f64_rem(<2 x double> %a, <2 x double> %b) {
entry:
  %r = frem <2 x double> %a, %b
  ret <2 x double> %r
}

; CHECK-LABEL: define dso_local <4 x float> @v4f32_srem(<4 x i32> %a, <4 x i32> %b)
; CHECK-NOT: srem
; CHECK: %[[SDIV:.+]] = sdiv <4 x i32> %a, %b
; CHECK: %[[MUL:.+]] = mul <4 x i32> %[[SDIV]], %b
; CHECK: %[[SUB:.+]] = sub <4 x i32> %a, %[[MUL]]
; CHECK: ret <4 x i32> %[[SUB]]
define dso_local <4 x i32> @v4i32_srem(<4 x i32> %a, <4 x i32> %b) {
entry:
  %r = srem <4 x i32> %a, %b
  ret <4 x i32> %r
}

; ============================================================
; Different widths and scalar FP kinds
; ============================================================

; CHECK-LABEL: define dso_local float @f32_rem_const(float %a)
; CHECK-NOT: frem
; CHECK: %[[DIV:.+]] = fdiv float %a, 2.000000e+00
; CHECK: %[[MUL:.+]] = fmul float %[[DIV]], 2.000000e+00
; CHECK: %[[SUB:.+]] = fsub float %a, %[[MUL]]
; CHECK: ret float %[[SUB]]
define dso_local float @f32_rem_const(float %a) {
entry:
  %r = frem float %a, 2.000000e+00
  ret float %r
}

; CHECK-LABEL: define dso_local fp128 @f128_rem(fp128 %a, fp128 %b)
; CHECK-NOT: frem
; CHECK: %[[DIV:.+]] = fdiv fp128 %a, %b
; CHECK: %[[MUL:.+]] = fmul fp128 %[[DIV]], %b
; CHECK: %[[SUB:.+]] = fsub fp128 %a, %[[MUL]]
; CHECK: ret fp128 %[[SUB]]
define dso_local fp128 @f128_rem(fp128 %a, fp128 %b) {
entry:
  %r = frem fp128 %a, %b
  ret fp128 %r
}

; ============================================================
; Fast-math flags preservation
; ============================================================

; CHECK-LABEL: define dso_local float @fastmath_rem(float %a, float %b)
; CHECK-NOT: frem
; CHECK: %[[DIV:.+]] = fdiv float %a, %b
; CHECK: %[[MUL:.+]] = fmul float %[[DIV]], %b
; CHECK: %[[SUB:.+]] = fsub float %a, %[[MUL]]
; CHECK: ret float %[[SUB]]
define dso_local float @fastmath_rem(float %a, float %b) {
entry:
  ; fast-math on frem (represented as metadata in textual IR with fast-math flags)
  %r = frem float %a, %b fast
  ret float %r
}

; ============================================================
; Multiple uses and chaining
; ============================================================

; CHECK-LABEL: define dso_local i32 @multi_use(i32 %a, i32 %b)
; CHECK-NOT: srem
; CHECK: %[[SDIV:.+]] = sdiv i32 %a, %b
; CHECK: %[[MUL:.+]] = mul i32 %[[SDIV]], %b
; CHECK: %[[SUB:.+]] = sub i32 %a, %[[MUL]]
; CHECK: %[[ADD:.+]] = add i32 %[[SUB]], %[[SUB]]
; CHECK: ret i32 %[[ADD]]
define dso_local i32 @multi_use(i32 %a, i32 %b) {
entry:
  %r = srem i32 %a, %b
  %x = add i32 %r, %r
  ret i32 %x
}

; CHECK-LABEL: define dso_local i32 @chain(i32 %a, i32 %b)
; CHECK-NOT: srem
; CHECK: %[[SDIV1:.+]] = sdiv i32 %a, %b
; CHECK: %[[MUL1:.+]] = mul i32 %[[SDIV1]], %b
; CHECK: %[[SUB1:.+]] = sub i32 %a, %[[MUL1]]
; CHECK: %[[SDIV2:.+]] = sdiv i32 %[[SUB1]], %b
; CHECK: %[[MUL2:.+]] = mul i32 %[[SDIV2]], %b
; CHECK: %[[SUB2:.+]] = sub i32 %[[SUB1]], %[[MUL2]]
; CHECK: ret i32 %[[SUB2]]
define dso_local i32 @chain(i32 %a, i32 %b) {
entry:
  %r1 = srem i32 %a, %b
  %r2 = srem i32 %r1, %b
  ret i32 %r2
}

; ============================================================
; Edge cases: zero divisor and constants (we only check decomposition)
; ============================================================

; CHECK-LABEL: define dso_local i32 @zero_div(i32 %a)
; CHECK-NOT: srem
; CHECK: %[[SDIV:.+]] = sdiv i32 %a, 0
; CHECK: %[[MUL:.+]] = mul i32 %[[SDIV]], 0
; CHECK: %[[SUB:.+]] = sub i32 %a, %[[MUL]]
; CHECK: ret i32 %[[SUB]]
define dso_local i32 @zero_div(i32 %a) {
entry:
  %r = srem i32 %a, 0
  ret i32 %r
}
