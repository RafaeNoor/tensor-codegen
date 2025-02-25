//===- Canonicalizer.cpp - Canonicalize MLIR operations -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This transformation pass converts operations into their canonical forms by
// folding constants, applying operation identity transformations etc.
//
//===----------------------------------------------------------------------===//

#include "PassDetail.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;

namespace {
/// Canonicalize operations in nested regions.
struct Canonicalizer : public CanonicalizerBase<Canonicalizer> {
  Canonicalizer(const GreedyRewriteConfig &config) : config(config) {}

  Canonicalizer() {
    // Default constructed Canonicalizer takes its settings from command line
    // options.
    config.useTopDownTraversal = topDownProcessingEnabled;
    config.enableRegionSimplification = enableRegionSimplification;
    config.maxIterations = maxIterations;
  }

  /// Initialize the canonicalizer by building the set of patterns used during
  /// execution.
  LogicalResult initialize(MLIRContext *context) override {
    RewritePatternSet owningPatterns(context);
    for (auto *dialect : context->getLoadedDialects())
      dialect->getCanonicalizationPatterns(owningPatterns);
    for (auto *op : context->getRegisteredOperations())
      op->getCanonicalizationPatterns(owningPatterns, context);
    patterns = std::move(owningPatterns);
    return success();
  }
  void runOnOperation() override {
    (void)applyPatternsAndFoldGreedily(getOperation()->getRegions(), patterns,
                                       config);
  }

  GreedyRewriteConfig config;
  FrozenRewritePatternSet patterns;
};
} // end anonymous namespace

/// Create a Canonicalizer pass.
std::unique_ptr<Pass> mlir::createCanonicalizerPass() {
  return std::make_unique<Canonicalizer>();
}

/// Creates an instance of the Canonicalizer pass with the specified config.
std::unique_ptr<Pass>
mlir::createCanonicalizerPass(const GreedyRewriteConfig &config) {
  return std::make_unique<Canonicalizer>(config);
}
