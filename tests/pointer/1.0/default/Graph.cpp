#include "Graph.h"
#include <android-base/logging.h>

#define PUSH_ERROR_IF(__cond__) if(__cond__) { errors.push_back(std::to_string(__LINE__) + ": " + #__cond__); }

namespace android {
namespace hardware {
namespace tests {
namespace pointer {
namespace V1_0 {
namespace implementation {

static void simpleGraph(IGraph::Graph& g) {
    g.nodes.resize(2);
    g.edges.resize(1);
    g.nodes[0].data = 10;
    g.nodes[1].data = 20;
    g.edges[0].left = &g.nodes[0];
    g.edges[0].right = &g.nodes[1];
}

static bool isSimpleGraph(const IGraph::Graph &g) {
    if(g.nodes.size() != 2) return false;
    if(g.edges.size() != 1) return false;
    if(g.nodes[0].data != 10) return false;
    if(g.nodes[1].data != 20) return false;
    if(g.edges[0].left != &g.nodes[0]) return false;
    if(g.edges[0].right != &g.nodes[1]) return false;
    return true;
}

static void logSimpleGraph(const char *prefix, const IGraph::Graph& g) {
    ALOGI("%s Graph %p, %d nodes, %d edges", prefix, &g, (int)g.nodes.size(), (int)g.edges.size());
    std::ostringstream os;
    for(size_t i = 0; i < g.nodes.size(); i++)
      os << &g.nodes[i] << " = " << g.nodes[i].data << ", ";
    ALOGI("%s Nodes: [%s]", prefix, os.str().c_str());
    os.str("");
    os.clear();
    for(size_t i = 0; i < g.edges.size(); i++)
      os << g.edges[i].left << " -> " << g.edges[i].right << ", ";
    ALOGI("%s Edges: [%s]", prefix, os.str().c_str());
}

// Methods from ::android::hardware::tests::pointer::V1_0::IGraph follow.
Return<void> Graph::passAGraph(const IGraph::Graph& g) {
    ALOGI("SERVER(Graph) passAGraph start.");
    PUSH_ERROR_IF(!isSimpleGraph(g));
    // logSimpleGraph("SERVER(Graph) passAGraph:", g);
    return Void();
}

Return<void> Graph::giveAGraph(giveAGraph_cb _cb) {
    IGraph::Graph g;
    simpleGraph(g);
    _cb(g);
    return Void();
}

Return<void> Graph::passANode(const IGraph::Node& n) {
    PUSH_ERROR_IF(n.data != 10);
    return Void();
}

Return<void> Graph::passTwoGraphs(IGraph::Graph const* g1, IGraph::Graph const* g2) {
    PUSH_ERROR_IF(g1 != g2);
    PUSH_ERROR_IF(!isSimpleGraph(*g1));
    logSimpleGraph("SERVER(Graph): passTwoGraphs", *g2);
    return Void();
}

Return<void> Graph::passAGamma(const IGraph::Gamma& c) {
    if(c.a_ptr == nullptr && c.b_ptr == nullptr)
      return Void();
    ALOGI("SERVER(Graph) passAGamma received c.a = %p, c.b = %p, c.a->s = %p, c.b->s = %p",
        c.a_ptr, c.b_ptr, c.a_ptr->s_ptr, c.b_ptr->s_ptr);
    ALOGI("SERVER(Graph) passAGamma received data %d, %d",
        (int)c.a_ptr->s_ptr->data, (int)c.b_ptr->s_ptr->data);
    PUSH_ERROR_IF(c.a_ptr->s_ptr != c.b_ptr->s_ptr);
    return Void();
}
Return<void> Graph::passASimpleRef(const IGraph::Alpha * a_ptr) {
    ALOGI("SERVER(Graph) passASimpleRef received %d", a_ptr->s_ptr->data);
    PUSH_ERROR_IF(a_ptr->s_ptr->data != 500);
    return Void();
}
Return<void> Graph::passASimpleRefS(const IGraph::Theta * s_ptr) {
    ALOGI("SERVER(Graph) passASimpleRefS received %d @ %p", s_ptr->data, s_ptr);
    PUSH_ERROR_IF(s_ptr->data == 10);
    return Void();
}
Return<void> Graph::giveASimpleRef(giveASimpleRef_cb _cb) {
    IGraph::Theta s; s.data = 500;
    IGraph::Alpha a; a.s_ptr = &s;
    _cb(&a);
    return Void();
}

Return<int32_t> Graph::getErrors() {
    if(!errors.empty()) {
        for(const auto& e : errors)
            ALOGW("SERVER(Graph) error: %s", e.c_str());
    }
    return errors.size();
}

IGraph* HIDL_FETCH_IGraph(const char* /* name */) {
    return new Graph();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace pointer
}  // namespace tests
}  // namespace hardware
}  // namespace android
