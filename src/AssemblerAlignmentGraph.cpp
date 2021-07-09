// Shasta.
#include "Assembler.hpp"
#include "LocalAlignmentGraph.hpp"
#include "Reads.hpp"
using namespace shasta;

// Standard libraries.
#include "chrono.hpp"
#include <queue>



// Create a local alignment graph starting from a given oriented read
// and walking out a given distance on the global alignment graph.
// An alignment graph is an undirected graph in which each vertex
// represents an oriented read. Two vertices are joined by an
// undirected edge if we have a found a good alignment,
// and stored it, between the corresponding oriented reads.

// The alignment graph is being phased out in favor of the read graph
// (see AssemblerReadGraph.cpp).

bool Assembler::createLocalAlignmentGraph(
    OrientedReadId orientedReadIdStart,
    size_t minAlignedMarkerCount,   // Minimum number of alignment markers to generate an edge.
    size_t maxTrim,                 // Maximum left/right trim (expressed in bases) to generate an edge.
    uint32_t maxDistance,           // How far to go from starting oriented read.
    double timeout,                 // Or 0 for no timeout.
    LocalAlignmentGraph& graph)
{
    const auto startTime = steady_clock::now();

    // Add the starting vertex.
    graph.addVertex(orientedReadIdStart,
        uint32_t(reads->getRead(orientedReadIdStart.getReadId()).baseCount), 0);

    // Initialize a BFS starting at the start vertex.
    std::queue<OrientedReadId> q;
    q.push(orientedReadIdStart);



    // Do the BFS.
    while(!q.empty()) {

        // See if we exceeded the timeout.
        if(seconds(steady_clock::now() - startTime) > timeout) {
            graph.clear();
            return false;
        }

        // Dequeue a vertex.
        const OrientedReadId orientedReadId0 = q.front();
        // cout << "Dequeued " << orientedReadId0;
        // cout << " with " << alignmentTable.size(orientedReadId0.getValue()) << " overlaps." << endl;
        q.pop();
        const uint32_t distance0 = graph.getDistance(orientedReadId0);
        const uint32_t distance1 = distance0 + 1;

        // Loop over overlaps/alignments involving this vertex.
        for(const uint64_t i: alignmentTable[orientedReadId0.getValue()]) {
            SHASTA_ASSERT(i < alignmentData.size());
            const AlignmentData& ad = alignmentData[i];

            // If the alignment involves too few markers, skip.
            if(ad.info.markerCount < minAlignedMarkerCount) {
                continue;
            }

            // If the trim does not satisfy our criteria, skip it.
            // const OrientedReadId overlapOrientedReadId0(ad.readIds[0], 0);
            // const OrientedReadId overlapOrientedReadId1(ad.readIds[1], ad.isSameStrand ? 0 : 1);
            uint32_t leftTrim;
            uint32_t rightTrim;
            tie(leftTrim, rightTrim) = ad.info.computeTrim();
            if(leftTrim>maxTrim || rightTrim>maxTrim) {
                continue;
            }

            // The overlap and the alignment satisfy our criteria.
            // Get the other oriented read involved in this overlap.
            const OrientedReadId orientedReadId1 = ad.getOther(orientedReadId0);


            // Update our BFS.
            // Note that we are pushing to the queue vertices at maxDistance,
            // so we can find all of their edges to other vertices at maxDistance.
            if(distance0 < maxDistance) {
                if(!graph.vertexExists(orientedReadId1)) {
                    graph.addVertex(orientedReadId1,
                        uint32_t(reads->getRead(orientedReadId1.getReadId()).baseCount), distance1);
                    q.push(orientedReadId1);
                }
                graph.addEdge(orientedReadId0, orientedReadId1,
                    ad.info);
            } else {
                SHASTA_ASSERT(distance0 == maxDistance);
                if(graph.vertexExists(orientedReadId1)) {
                    graph.addEdge(orientedReadId0, orientedReadId1,
                        ad.info);
                }
            }


#if 0
            // THE OLD CODE DOES NOT CREATE EDGES
            // BETWEEN VERTICES AT MAXDISTANCE.
            // Add the vertex for orientedReadId1, if necessary.
            // Also add orientedReadId1 to the queue, unless
            // we already reached the maximum distance.
            if(!graph.vertexExists(orientedReadId1)) {
                graph.addVertex(orientedReadId1,
                    uint32_t(reads->getRead(orientedReadId1.getReadId()).baseCount), distance1);
                if(distance1 < maxDistance) {
                    q.push(orientedReadId1);
                    // cout << "Enqueued " << orientedReadId1 << endl;
                }
            }

            // Add the edge.
            graph.addEdge(orientedReadId0, orientedReadId1,
                ad.info);
#endif
        }

    }

    return true;
}
