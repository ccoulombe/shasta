// Shasta.
#include "LocalAlignmentCandidateGraph.hpp"
#include "writeGraph.hpp"
using namespace shasta;

// Boost libraries.
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

using boost::graph_traits;
using boost::adjacent_vertices;
using boost::edge;

// Standard library.
#include <fstream.hpp>
#include <stdexcept.hpp>

using std::tie;


void LocalAlignmentCandidateGraph::addVertex(
    OrientedReadId orientedReadId,
    uint32_t baseCount,
    uint32_t distance)
{
    // Check that we don't already have a vertex with this OrientedReadId.
    SHASTA_ASSERT(vertexMap.find(orientedReadId) == vertexMap.end());

    // Create the vertex.
    const vertex_descriptor v = add_vertex(LocalAlignmentCandidateGraphVertex(orientedReadId, baseCount, distance), *this);

    // Store it in the vertex map.
    vertexMap.insert(make_pair(orientedReadId, v));
}



void LocalAlignmentCandidateGraph::addEdge(
    OrientedReadId orientedReadId0,
    OrientedReadId orientedReadId1,
    bool inCandidates,
    bool inAlignments,
    bool inReadgraph,
    bool inReferenceAlignments)
{
    // Find the vertices corresponding to these two OrientedReadId.
    const auto it0 = vertexMap.find(orientedReadId0);
    SHASTA_ASSERT(it0 != vertexMap.end());
    const vertex_descriptor v0 = it0->second;
    const auto it1 = vertexMap.find(orientedReadId1);
    SHASTA_ASSERT(it1 != vertexMap.end());
    const vertex_descriptor v1 = it1->second;

    // Add the edge.
    add_edge(v0, v1, LocalAlignmentCandidateGraphEdge(inCandidates, inAlignments, inReadgraph, inReferenceAlignments), *this);
}


void LocalAlignmentCandidateGraph::getAdjacentReadIds(OrientedReadId id, vector<OrientedReadId>& adjacentReadIds){
    LocalAlignmentCandidateGraph& graph = *this;

    const auto it = vertexMap.find(id);

    // This read may not exist in the ReferenceGraph at all (in which case, do nothing)
    if (it != vertexMap.end()) {
        const vertex_descriptor v = it->second;

        BGL_FORALL_ADJ(v, v2, graph, LocalAlignmentCandidateGraph){
            OrientedReadId otherId = OrientedReadId::fromValue(graph[v2].orientedReadId);
            adjacentReadIds.push_back(otherId);

        }
    }
}


uint32_t LocalAlignmentCandidateGraph::getDistance(OrientedReadId orientedReadId) const
{
    const auto it = vertexMap.find(orientedReadId);
    SHASTA_ASSERT(it != vertexMap.end());
    const vertex_descriptor v = it->second;
    return (*this)[v].distance;
}


bool LocalAlignmentCandidateGraph::vertexExists(OrientedReadId orientedReadId) const
{
   return vertexMap.find(orientedReadId) != vertexMap.end();
}


bool LocalAlignmentCandidateGraph::edgeExists(OrientedReadId a, OrientedReadId b) const
{
    auto resultA = vertexMap.find(a);
    auto resultB = vertexMap.find(b);

    bool exists = false;
    if (resultA != vertexMap.end() and resultB != vertexMap.end()){
        exists = edge(resultA->second, resultB->second, *this).second;
    }

    return exists;
}


// Write the graph in Graphviz format.
void LocalAlignmentCandidateGraph::write(const string& fileName, uint32_t maxDistance) const
{
    ofstream outputFileStream(fileName);
    if(!outputFileStream) {
        throw runtime_error("Error opening " + fileName);
    }
    write(outputFileStream, maxDistance);
}


void LocalAlignmentCandidateGraph::write(ostream& s, uint32_t maxDistance) const
{
    Writer writer(*this, maxDistance);
    boost::write_graphviz(s, *this, writer, writer, writer,
        boost::get(&LocalAlignmentCandidateGraphVertex::orientedReadId, *this));
}


LocalAlignmentCandidateGraph::Writer::Writer(
    const LocalAlignmentCandidateGraph& graph,
    uint32_t maxDistance) :
    graph(graph),
    maxDistance(maxDistance)
{
}



void LocalAlignmentCandidateGraph::Writer::operator()(std::ostream& s) const
{
    s << "layout=sfdp;\n";
    s << "ratio=expand;\n";
    s << "node [shape=point];\n";
    s << "edge [penwidth=\"0.2\"];\n";

    // This turns off the tooltip on the graph.
    s << "tooltip = \" \";\n";
}


void LocalAlignmentCandidateGraph::Writer::operator()(std::ostream& s, vertex_descriptor v) const
{
    const LocalAlignmentCandidateGraphVertex& vertex = graph[v];
    const OrientedReadId orientedReadId = OrientedReadId::fromValue(vertex.orientedReadId);

    s << "[";
    s << " tooltip=\"" << orientedReadId << " length " << vertex.baseCount << " distance " << vertex.distance << "\"";
    s << " URL=\"exploreRead?readId=" << orientedReadId.getReadId();
    s << "&strand=" << orientedReadId.getStrand() << "\"";
    s << " width=" << sqrt(1.e-6 * vertex.baseCount);
    if(vertex.distance == 0) {
        s << " color=lightGreen fillcolor=lightGreen";
    } else if(vertex.distance == maxDistance) {
            s << " color=cyan fillcolor=cyan";
    }
    s << "]";
}



void LocalAlignmentCandidateGraph::Writer::operator()(std::ostream& s, edge_descriptor e) const
{
//    const LocalAlignmentCandidateGraphEdge& edge = graph[e];
    const vertex_descriptor v0 = source(e, graph);
    const vertex_descriptor v1 = target(e, graph);
    const LocalAlignmentCandidateGraphVertex& vertex0 = graph[v0];
    const LocalAlignmentCandidateGraphVertex& vertex1 = graph[v1];

    s << "[";
    s << "tooltip=\"" << OrientedReadId::fromValue(vertex0.orientedReadId) << " ";
    s << OrientedReadId::fromValue(vertex1.orientedReadId) << "\"";
    s << "]";
}




// Compute sfdp layout using graphviz and store the results
// in the vertex positions.
ComputeLayoutReturnCode LocalAlignmentCandidateGraph::computeLayout(
    const string& layoutMethod,
    double timeout)
{
    LocalAlignmentCandidateGraph& graph = *this;

    // Compute the layout.
    std::map<vertex_descriptor, array<double, 2> > positionMap;
    const ComputeLayoutReturnCode returnCode =
        shasta::computeLayout(graph, layoutMethod, timeout, positionMap);
    if(returnCode != ComputeLayoutReturnCode::Success) {
        return returnCode;
    }

    // Store it in the vertices.
    BGL_FORALL_VERTICES(v, graph, LocalAlignmentCandidateGraph) {
        const auto it = positionMap.find(v);
        SHASTA_ASSERT(it != positionMap.end());
        graph[v].position = it->second;
    }
    return ComputeLayoutReturnCode::Success;
}


string LocalAlignmentCandidateGraphEdge::getSvgClassName() const{
    string className;

    if (inReadGraph){
        if (inReferenceAlignments){
            className = "ReadGraphInRef";
        }
        else{
            className = "ReadGraph";
        }
    }
    else if (inAlignments){
        if (inReferenceAlignments){
            className = "AlignmentInRef";
        }
        else{
            className = "Alignment";
        }
    }
    else if(inCandidates){
        if (inReferenceAlignments){
            className = "CandidateInRef";
        }
        else{
            className = "Candidate";
        }
    }
    else {
        if (inReferenceAlignments){
            className = "ReferenceOnly";
        }
        else{
            throw runtime_error("ERROR: edge in candidate graph does not have any class label");
        }
    }

    return className;
}

uint8_t LocalAlignmentCandidateGraphEdge::getSvgOrdering() const{
    return  uint8_t(int(inAlignments) + int(inReadGraph) + int(inReferenceAlignments));
}


uint8_t LocalAlignmentCandidateGraphVertex::getSvgOrdering() const{
    return 0;
}


// Write directly to svg, without using Graphviz rendering.
// This assumes that the layout was already computed
// and stored in the vertices.
void LocalAlignmentCandidateGraph::writeSvg(
    const string& svgId,
    uint64_t width,
    uint64_t height,
    double vertexScalingFactor,
    double edgeThicknessScalingFactor,
    uint64_t maxDistance,
    ostream& svg) const
{
    using Graph = LocalAlignmentCandidateGraph;
    using VertexAttributes = WriteGraph::VertexAttributes;
    using EdgeAttributes = WriteGraph::EdgeAttributes;
    const Graph& graph = *this;


    // Fill in vertex attributes.
    std::map<vertex_descriptor, VertexAttributes> vertexAttributes;
    BGL_FORALL_VERTICES(v, graph, Graph) {
        const auto& vertex = graph[v];
        const OrientedReadId orientedReadId = OrientedReadId::fromValue(vertex.orientedReadId);
        VertexAttributes attributes;

        attributes.radius = vertexScalingFactor * 0.03;

        attributes.id = "Vertex-" + orientedReadId.getString();

        if(vertex.distance == 0) {
            attributes.color = "lime";
        } else if(vertex.distance == maxDistance) {
            attributes.color = "cyan";
        }

        attributes.tooltip = "Read " + orientedReadId.getString();

        attributes.url = "exploreRead?readId=" + to_string(orientedReadId.getReadId()) +
            "&strand=" + to_string(orientedReadId.getStrand());

        vertexAttributes.insert(make_pair(v, attributes));
    }


    // Fill in edge attributes.
    std::map<edge_descriptor, EdgeAttributes> edgeAttributes;
    BGL_FORALL_EDGES(e, graph, Graph) {
        const auto& edge = graph[e];
        const vertex_descriptor v0 = source(e, graph);
        const vertex_descriptor v1 = target(e, graph);
        const auto& vertex0 = graph[v0];
        const auto& vertex1 = graph[v1];

        EdgeAttributes attributes;

        attributes.thickness = edgeThicknessScalingFactor * 2.e-3;

        // Ratchet up the color towards green for each successive filter it passes (candidate -> alignment -> readgraph)
        attributes.color = "#450BBA";

        if (edge.inAlignments){
            attributes.color = "#0658C2";
        }
        if (edge.inReadGraph){
            attributes.color = "#00C442";
        }
        if (edge.inReferenceAlignments and not edge.inCandidates){
            attributes.color = "#FF2800";
        }

        attributes.tooltip =
            OrientedReadId::fromValue(vertex0.orientedReadId).getString() + " " +
            OrientedReadId::fromValue(vertex1.orientedReadId).getString();

        edgeAttributes.insert(make_pair(e, attributes));
    }

    // Write to svg.
    WriteGraph::writeOrderedSvg(graph, svgId, width, height,
        vertexAttributes, edgeAttributes, svg);
}
