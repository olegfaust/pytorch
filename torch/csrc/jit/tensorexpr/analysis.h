#pragma once

#include <torch/csrc/jit/tensorexpr/ir.h>
#include <torch/csrc/jit/tensorexpr/ir_visitor.h>
#include <torch/csrc/jit/tensorexpr/stmt.h>
#include <torch/csrc/jit/tensorexpr/tensor.h>

namespace torch {
namespace jit {
namespace tensorexpr {
class HasRand : public IRVisitor {
 public:
  HasRand(Stmt* stmt) : stmt_(stmt) {
    stmt_->accept(this);
  }

  bool has_rand() const {
    return has_rand_;
  }

 private:
  void visit(const Intrinsics* v) override {
    if (v->op_type() == IntrinsicsOp::kRand) {
      has_rand_ = true;
    } else {
      IRVisitor::visit(v);
    }
  }
  Stmt* stmt_;
  bool has_rand_ = false;
};

template <typename Node>
class NodeFinder : public IRVisitor {
 public:
  virtual void visit(const Node* v) override {
    nodes.push_back((Node*)v);
    IRVisitor::visit(v);
  }

  static std::vector<Node*> find(Stmt* s) {
    NodeFinder<Node> nf;
    s->accept(&nf);
    return nf.nodes;
  }

  std::vector<Node*> nodes;
};

class VarFinder : public IRVisitor {
 public:
  virtual void visit(const Var* v) override {
    vars_.insert(v);
    IRVisitor::visit(v);
  }

  static std::unordered_set<const Var*> find(Stmt* s) {
    VarFinder nf;
    s->accept(&nf);
    return nf.vars();
  }

  const std::unordered_set<const Var*>& vars() {
    return vars_;
  }

 private:
  std::unordered_set<const Var*> vars_;
};

// A class that analyzes the given program relevant for Block backend
// It creates a map of multi dim buffers and their flat verions
class CreateBufferMap : public IRVisitor {
 public:
  const std::unordered_map<std::string, const Buf*>& getBufferMap() const {
    return map_input_to_tensor_bufs_;
  }

 private:
  void visit(const Store* v) override {
    auto load_node = dynamic_cast<const Load*>(v->value());
    auto call_node = dynamic_cast<const FunctionCall*>(v->value());
    if (load_node || call_node) {
      TORCH_INTERNAL_ASSERT(!(load_node && call_node));
      auto t_buf = load_node ? load_node->buf() : call_node->tensor()->buf();
      if (load_node) {
        map_input_to_tensor_bufs_.emplace(t_buf->name_hint(), v->buf());
      } else {
        map_input_to_tensor_bufs_.emplace(v->buf()->name_hint(), t_buf);
      }
    }
    v->value()->accept(this);
  }
  std::unordered_map<std::string, const Buf*> map_input_to_tensor_bufs_;
};
} // namespace tensorexpr
} // namespace jit
} // namespace torch
