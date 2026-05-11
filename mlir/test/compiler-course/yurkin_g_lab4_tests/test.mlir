// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/yurkin_g_lab4_MLIR%shlibext
// --pass-
// --pass-pipeline="builtin.module(yurkin-condition-tracer)" %s | FileCheck %s
// CHECK: func.func @trace_condition_then_begin()
// CHECK: func.func @trace_condition_then_end()
// CHECK: func.func @trace_condition_else_begin()
// CHECK: func.func @trace_condition_else_end()
// CHECK-LABEL: func.func @test_scf()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK: func.call @trace_condition_else_begin()
// CHECK: func.call @trace_condition_else_end()
// CHECK-LABEL: func.func @test_affine()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK: func.call @trace_condition_else_begin()
// CHECK: func.call @trace_condition_else_end()
// CHECK-LABEL: func.func @test_scf_no_else()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK-LABEL: func.func @test_affine_no_else()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK-LABEL: func.func @test_nested()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK: func.call @trace_condition_then_end()
// CHECK-LABEL: func.func @test_scf_for()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK-LABEL: func.func @test_affine_for()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK-LABEL: func.func @test_if_with_result()
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
module {
  func.func @test_scf() {
    % c = arith.constant true : i1 scf.if % c {
      % t = arith.constant 42 : i32 scf.yield
    }
    else {
      % e = arith.constant 0 : i32 scf.yield
    }
    return
  }
  func.func @test_affine() {
    % c = arith.constant true : i1 affine.if % c {
      % t = arith.constant 1 : i32 affine.yield
    }
    else {
      % e = arith.constant 2 : i32 affine.yield
    }
    return
  }
  func.func @test_scf_no_else() {
    % c = arith.constant true : i1 scf.if % c {
      % t = arith.constant 7 : i32 scf.yield
    }
    return
  }
  func.func @test_affine_no_else() {
    % c = arith.constant true : i1 affine.if % c {
      % t = arith.constant 8 : i32 affine.yield
    }
    return
  }
  func.func @test_nested() {
    % c1 = arith.constant true : i1 scf.if % c1 {
      % c2 = arith.constant true : i1 scf.if % c2 {
        % x = arith.constant 1 : i32 scf.yield
      }
      else { % y = arith.constant 2 : i32 scf.yield} scf.yield
    }
    else {
      % z = arith.constant 0 : i32 scf.yield
    }
    return
  }
  func.func @test_scf_for() {
    %c0 = arith.constant 0 : index
    %c10 = arith.constant 10 : index
    %c1 = arith.constant 1 : index
    scf.for %i = %c0 to %c10 step %c1 {
      % t = arith.constant true : i1 scf.if %
            t{ % v = arith.constant 5 : i32 scf.yield} scf.yield
    }
    return
  }
  func.func @test_affine_for() {
    affine.for %i = 0 to 4 {
      % t = arith.constant true : i1 affine.if %
            t{ % v = arith.constant 3 : i32 affine.yield} affine.yield
    }
    return
  }
  func.func @test_if_with_result()->i32 {
    % c = arith.constant true : i1 % r = scf.if % c->(i32) {
      % t = arith.constant 11 : i32 scf.yield % t : i32
    }
    else {
      % e = arith.constant 22 : i32 scf.yield % e : i32
    }
    return
  }
}
