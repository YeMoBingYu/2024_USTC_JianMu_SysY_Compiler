#pragma once

#include "BasicBlock.hpp"
#include "PassManager.hpp"

#include <map>
#include <set>

class Dominators : public Pass
{
public:
  using BBSet = std::set<BasicBlock *>;

  explicit Dominators(Module *m) : Pass(m) {}
  ~Dominators() = default;
  void run() override;

  BasicBlock *get_idom(BasicBlock *bb) { return idom_.at(bb); }
  const BBSet &get_dominance_frontier(BasicBlock *bb)
  {
    return dom_frontier_[bb];
  }
  const BBSet &get_dom(BasicBlock *bb)
  {
    return dom_[bb];
  }

private:
  void create_idom(Function *f);
  void create_dominance_frontier(Function *f);
  void create_dom(Function *f);

  std::map<BasicBlock *, BBSet> dom_{};          // 支配集合
  std::map<BasicBlock *, BasicBlock *> idom_{};  // 直接支配
  std::map<BasicBlock *, BBSet> dom_frontier_{}; // 支配边界集合
};
