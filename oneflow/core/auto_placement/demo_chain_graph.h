#ifndef ONEFLOW_CORE_AUTO_PLACEMENT_DEMO_CHAIN_GRAPH_H_
#define ONEFLOW_CORE_AUTO_PLACEMENT_DEMO_CHAIN_GRAPH_H_
#include "oneflow/core/graph/graph.h"
#include "oneflow/core/job/task.pb.h"
#include "oneflow/core/graph/graph_node_visitor_util.h"

namespace oneflow {

namespace df {

class DemoChainNode;
using ChainNodeVisitor = GraphNodeVisitorUtil<const DemoChainNode*>;
using ChainNodeHandler =
    GraphNodeVisitorUtil<const DemoChainNode*>::HandlerType;
using IsReachablePredicator =
    std::function<bool(const DemoChainNode* src, const DemoChainNode* dst)>;

class DemoChainRegst final {
 public:
  OF_DISALLOW_COPY_AND_MOVE(DemoChainRegst);
  DemoChainRegst(DemoChainNode* producer, int64_t chain_regst_id)
      : producer_(producer), chain_regst_id_(chain_regst_id) {}
  ~DemoChainRegst() = default;

  void HandleDiff(DemoChainRegst* regst) const { diff_handler_(regst); }

  // Getters
  int64_t chain_regst_id() const { return chain_regst_id_; }
  const DemoChainNode* producer() const { return producer_; }
  const std::unordered_set<const DemoChainNode*>& consumers() const {
    return consumers_;
  }

  // Setters
  DemoChainNode* mut_producer() { return producer_; }
  void set_diff_handler(
      const std::function<void(DemoChainRegst*)>& diff_handler) {
    diff_handler_ = diff_handler;
  }
  void AddConsumer(const DemoChainNode* consumer) {
    consumers_.insert(consumer);
  }

 private:
  DemoChainNode* producer_;
  int64_t chain_regst_id_;
  std::function<void(DemoChainRegst*)> diff_handler_;
  std::unordered_set<const DemoChainNode*> consumers_;
};

class DemoChainEdge;

class DemoChainNode final : public Node<DemoChainNode, DemoChainEdge> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(DemoChainNode);
  DemoChainNode(const std::string& name, TaskType task_type)
      : name_(name), task_type_(task_type) {}
  ~DemoChainNode() = default;

  const std::string& name() const { return name_; }
  TaskType task_type() const { return task_type_; }
  int64_t chain_node_id() const { return chain_node_id_; }
  int64_t fw_chain_node_id() const { return fw_chain_node_id_; }

  void set_name(const std::string& name) { name_ = name; }
  void set_chain_node_id(int64_t chain_node_id) {
    chain_node_id_ = chain_node_id;
  }
  void set_fw_chain_node_id(int64_t fw_chain_node_id) {
    fw_chain_node_id_ = fw_chain_node_id;
  }

 private:
  std::string name_;
  TaskType task_type_;
  int64_t chain_node_id_;
  int64_t fw_chain_node_id_;
};

class DemoChainNodeSubGraph final {
 public:
  OF_DISALLOW_COPY_AND_MOVE(DemoChainNodeSubGraph);
  DemoChainNodeSubGraph(
      const DemoChainNode* start_node,
      const std::unordered_set<const DemoChainNode*>& end_nodes,
      const IsReachablePredicator& is_reachable)
      : start_node_(start_node),
        end_nodes_(end_nodes),
        is_reachable_(&is_reachable) {}
  ~DemoChainNodeSubGraph() = default;
  void CalcLongestPath(
      std::vector<int64_t>* path,
      const std::function<double(int64_t)>& Time4ChainNodeId) const;

 private:
  void TopoForEachChainNode(
      const std::function<void(const DemoChainNode*)>& Handler) const;
  void ForEachInNode(
      const DemoChainNode* node,
      const std::function<void(const DemoChainNode*)>& Handler) const;
  void ForEachOutNode(
      const DemoChainNode* node,
      const std::function<void(const DemoChainNode*)>& Handler) const;
  bool IsReachable(const DemoChainNode* src, const DemoChainNode* dst) const {
    return (*is_reachable_)(src, dst);
  }
  bool IsReachableFromStartNode(const DemoChainNode* node) const;
  bool IsReachableToEndNode(const DemoChainNode* node) const;

  const DemoChainNode* start_node_;
  std::unordered_set<const DemoChainNode*> end_nodes_;
  const IsReachablePredicator* is_reachable_;
};

class DemoChainEdge final : public Edge<DemoChainNode, DemoChainEdge> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(DemoChainEdge);
  DemoChainEdge() = default;
  ~DemoChainEdge() = default;
};

class DemoChainGraph : public Graph<DemoChainNode, DemoChainEdge> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(DemoChainGraph);
  DemoChainGraph();
  virtual ~DemoChainGraph() = default;
  virtual void LogicalGraph();

  size_t FwChainNodeNum() const;
  size_t ChainNodeNum() const;

  std::vector<std::vector<int64_t>> CalcChainNodeId2FwChainNodeId() const;

  std::vector<std::vector<int64_t>> CalcChainRegstId2ProducerChainNodeId()
      const;

  std::vector<std::vector<int64_t>> CalcChainRegstId2PathChainNodeIds(
      const std::function<double(int64_t)>& GetTime) const;

  std::vector<std::vector<int64_t>> CalcChainRegstId2PathChainNodeIds() const {
    return CalcChainRegstId2PathChainNodeIds(
        [](int64_t) -> double { return 1; });
  }

 protected:
  DemoChainRegst* Op(const std::string& name,
                     std::vector<DemoChainRegst*> inputs);

  DemoChainRegst* Op(const std::string& name) { return Op(name, {}); }
  DemoChainRegst* Model(const std::string& name);
  void Loss(DemoChainRegst* regst);

 private:
  int64_t NewChainNodeId() { return ++chain_node_id_; }
  int64_t NewChainRegstId() { return ++chain_node_id_; }
  void InitIsReachable();
  void InitRegst2ChainNodeSubGraphs();
  IsReachablePredicator MakeIsReachablePredicator() const;
  void TopoForEachChainNode(
      const std::function<void(const DemoChainNode*)>& Handler) const;
  void ForEachInNode(
      const DemoChainNode* node,
      const std::function<void(const DemoChainNode*)>& Handler) const;
  void ForEachOutNode(
      const DemoChainNode* node,
      const std::function<void(const DemoChainNode*)>& Handler) const;
  DemoChainRegst* NewRegst(DemoChainNode* producer);
  DemoChainNode* NewChainNode(const std::string& name, TaskType task_type);
  DemoChainNode* NewForwardNode(const std::string& name);
  DemoChainNode* NewBackwardNode(const std::string& name);
  DemoChainNode* NewDiffAccNode(const std::string& name);
  DemoChainNode* NewMdUpdtNode(const std::string& name);
  void Consume(DemoChainNode* node, DemoChainRegst* regst);

  int64_t chain_node_id_;
  int64_t chain_regst_id_;
  std::list<std::unique_ptr<DemoChainRegst>> regsts_;
  IsReachablePredicator is_reachable_;
  HashMap<const DemoChainRegst*, std::unique_ptr<DemoChainNodeSubGraph>>
      regst2chain_node_sub_graph_;
};

}  // namespace df

}  // namespace oneflow

#endif  // ONEFLOW_CORE_AUTO_PLACEMENT_DEMO_CHAIN_GRAPH_H_
