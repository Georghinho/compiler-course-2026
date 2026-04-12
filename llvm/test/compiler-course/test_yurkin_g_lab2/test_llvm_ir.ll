; RUN: opt -load-pass-plugin %llvmshlibdir/example_LLVM_IR%pluginext \
; RUN: -passes=example -S %s | FileCheck %s

; Scalar floating-point remainder decomposition
; CHECK-LABEL: define dso_local double @rem_fp_scalar(double %x, double %y)
; CHECK-NOT: frem
; CHECK: %[[DIV_F:.+]] = fdiv double %x, %y
; CHECK: %[[MUL_F:.+]] = fmul double %[[DIV_F]], %y
; CHECK: %[[RES_F:.+]] = fsub double %x, %[[MUL_F]]
; CHECK: ret double %[[RES_F]]
define dso_local double @rem_fp_scalar(double %x, double %y) {
entry:
  %tmp = frem double %x, %y
  ret double %tmp
}

; Signed integer remainder decomposition (32-bit)
; CHECK-LABEL: define dso_local i32 @rem_s32(i32 %p, i32 %q)
; CHECK-NOT: srem
; CHECK: %[[DIV_S:.+]] = sdiv i32 %p, %q
; CHECK: %[[MUL_S:.+]] = mul i32 %[[DIV_S]], %q
; CHECK: %[[RES_S:.+]] = sub i32 %p, %[[MUL_S]]
; CHECK: ret i32 %[[RES_S]]
define dso_local i32 @rem_s32(i32 %p, i32 %q) {
entry:
  %tmp = srem i32 %p, %q
  ret i32 %tmp
}

; Unsigned integer remainder decomposition (64-bit)
; CHECK-LABEL: define dso_local i64 @rem_u64(i64 %a, i64 %b)
; CHECK-NOT: urem
; CHECK: %[[DIV_U:.+]] = udiv i64 %a, %b
; CHECK: %[[MUL_U:.+]] = mul i64 %[[DIV_U]], %b
; CHECK: %[[RES_U:.+]] = sub i64 %a, %[[MUL_U]]
; CHECK: ret i64 %[[RES_U]]
define dso_local i64 @rem_u64(i64 %a, i64 %b) {
entry:
  %tmp = urem i64 %a, %b
  ret i64 %tmp
}

; Vector floating-point remainder (2 lanes)
; CHECK-LABEL: define dso_local <2 x float> @rem_v2f32(<2 x float> %v1, <2 x float> %v2)
; CHECK-NOT: frem
; CHECK: %[[DIV_V:.+]] = fdiv <2 x float> %v1, %v2
; CHECK: %[[MUL_V:.+]] = fmul <2 x float> %[[DIV_V]], %v2
; CHECK: %[[RES_V:.+]] = fsub <2 x float> %v1, %[[MUL_V]]
; CHECK: ret <2 x float> %[[RES_V]]
define dso_local <2 x float> @rem_v2f32(<2 x float> %v1, <2 x float> %v2) {
entry:
  %tmp = frem <2 x float> %v1, %v2
  ret <2 x float> %tmp
}

; Fast-math preservation check (fast flag before type)
; CHECK-LABEL: define dso_local float @rem_fast(float %a, float %b)
; CHECK-NOT: frem
; CHECK: %[[DIV_FM:.+]] = fdiv float %a, %b
; CHECK: %[[MUL_FM:.+]] = fmul float %[[DIV_FM]], %b
; CHECK: %[[RES_FM:.+]] = fsub float %a, %[[MUL_FM]]
; CHECK: ret float %[[RES_FM]]
define dso_local float @rem_fast(float %a, float %b) {
entry:
  ; fast-math flag placed before the type
  %tmp = frem fast float %a, %b
  ret float %tmp
}

; Multiple uses of remainder result
; CHECK-LABEL: define dso_local i32 @rem_multi(i32 %x, i32 %y)
; CHECK-NOT: srem
; CHECK: %[[DIV_M:.+]] = sdiv i32 %x, %y
; CHECK: %[[MUL_M:.+]] = mul i32 %[[DIV_M]], %y
; CHECK: %[[RES_M:.+]] = sub i32 %x, %[[MUL_M]]
; CHECK: %[[ADD_M:.+]] = add i32 %[[RES_M]], %[[RES_M]]
; CHECK: ret i32 %[[ADD_M]]
define dso_local i32 @rem_multi(i32 %x, i32 %y) {
entry:
  %r = srem i32 %x, %y
  %s = add i32 %r, %r
  ret i32 %s
}

; Chained remainders (result used in another remainder)
; CHECK-LABEL: define dso_local i32 @rem_chain(i32 %a, i32 %b)
; CHECK-NOT: srem
; CHECK: %[[DIV1:.+]] = sdiv i32 %a, %b
; CHECK: %[[MUL1:.+]] = mul i32 %[[DIV1]], %b
; CHECK: %[[RES1:.+]] = sub i32 %a, %[[MUL1]]
; CHECK: %[[DIV2:.+]] = sdiv i32 %[[RES1]], %b
; CHECK: %[[MUL2:.+]] = mul i32 %[[DIV2]], %b
; CHECK: %[[RES2:.+]] = sub i32 %[[RES1]], %[[MUL2]]
; CHECK: ret i32 %[[RES2]]
define dso_local i32 @rem_chain(i32 %a, i32 %b) {
entry:
  %r1 = srem i32 %a, %b
  %r2 = srem i32 %r1, %b
  ret i32 %r2
}
