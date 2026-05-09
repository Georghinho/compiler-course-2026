// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/yurkin_g_lab4_MLIR%shlibext --pass-pipeline="builtin.module(example_MLIR)" %s | FileCheck %s
// CHECK: func.call @trace_condition_then_begin()
// CHECK: func.call @trace_condition_then_end()
// CHECK: func.call @trace_condition_else_begin()
// CHECK: func.call @trace_condition_else_end()
module {
  func.func @test_scf() {
    %c = arith.constant true : i1
    scf.if %c {
      %t = arith.constant 42 : i32
      scf.yield
    } else {
      %e = arith.constant 0 : i32
      scf.yield
    }
    return
  }
  func.func @test_affine() {
    %c = arith.constant true : i1
    affine.if %c {
      %t = arith.constant 1 : i32
      affine.yield
    } else {
      %e = arith.constant 2 : i32
      affine.yield
    }
    return
  }
}

