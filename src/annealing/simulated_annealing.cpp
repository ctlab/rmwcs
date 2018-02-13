#include "simulated_annealing.h"

namespace {
    double probability(double e, double es, double temp) {
        return std::min(1.0, exp(es - e / temp));
    }
}

namespace annealing {

    SimulatedAnnealing::SimulatedAnnealing(const Graph &graph, RandomEngine& random_engine)
                                          :graph(graph),
                                           tokens{graph.edgeset_size()},
                                           random_engine(random_engine),
                                           degree{graph.size(), 0},
                                           unif{random_engine},
                                           dynamic_graph{graph.size()},
                                           module_vertices{graph.size(), random_engine},
                                           module_edges{graph.edgeset_size(), random_engine},
                                           boundary{graph.edgeset_size(), random_engine} {}

    void SimulatedAnnealing::run(CoolingSchedule& schedule) {
        while (schedule.is_hot()) {

        }
    }

    void SimulatedAnnealing::step(CoolingSchedule& schedule) {
        temperature = schedule.temperature();
        if (size == 0) {
            empty_module_step();
        } else {
            edge_step();
        }
        if (score > best_score) {
            score = best_score;
            best = Module(graph, module_vertices.content(), module_edges.content());
        }
    }

    void SimulatedAnnealing::add_vertex(size_t v) {
        size++;
        module_vertices.add(v);
        for (Edge e: graph.neighbours(v)) {
            size_t u = e.opposite(v);
            size_t e_id = e.num();
            if (!(module_edges.contains(e_id) || boundary.contains(e_id))) {
                boundary.add(e_id);
            }
        }
        score += graph.weight(v);
    }

    void SimulatedAnnealing::add_edge(size_t e) {
        boundary.remove(e);
        module_edges.add(e);
        Edge edge = graph.edge(e);
        size_t v = edge.from();
        size_t u = edge.to();
        ++degree[v];
        ++degree[u];
        if (!module_vertices.contains(v)) {
            add_vertex(v);
        }
        if (!module_vertices.contains(u)) {
            add_vertex(u);
        }
        tokens[e] = std::move(dynamic_graph.add(v, u));
        score += edge.weight();
    }

    bool SimulatedAnnealing::remove_edge(size_t e, size_t v, size_t u) {
        Edge edge = graph.edge(e);
        dynamic_graph.remove(std::move(tokens[e]));
        size_t comp_size = dynamic_graph.component_size(v);
        if (comp_size < size - 1 && comp_size != 1) {
            tokens[e] = std::move(dynamic_graph.add(v, u));
            return false;
        }
        if (comp_size == size) {
            boundary.add(e);
            return true;
        }
        if (comp_size == size - 1) {
            remove_vertex(u);
        } else {
            remove_vertex(v);
        }
        score -= edge.weight();
        --degree[v];
        --degree[u];
        return true;
    }

    void SimulatedAnnealing::remove_vertex(size_t v) {
        size--;
        for (Edge e: graph.neighbours(v)) {
            if (boundary.contains(e.num())) {
                boundary.remove(e.num());
            }
        }
        module_vertices.remove(v);
        score -= graph.weight(v);
    }

    void SimulatedAnnealing::empty_module_step() {
        size_t v = uniform(graph.size());
        if (accepts(graph.weight(v))) {
            add_vertex(v);
        }
    }

    bool SimulatedAnnealing::accepts(double diff) {
        double prob = probability(score, score + diff, temperature);
        return unif() < prob;
    }

    void SimulatedAnnealing::edge_step() {
        size_t bdr_sz = boundary.size();
        size_t mdl_sz = module_edges.size();
        size_t r = uniform(bdr_sz + mdl_sz);
        if (r < bdr_sz) {
            add_from_bdr();
        } else {
            remove_from_module();
        }
    }

    size_t SimulatedAnnealing::uniform(size_t n) {
        std::uniform_int_distribution<size_t> sampler(0, n - 1);
        return sampler(random_engine);
    }

    void SimulatedAnnealing::add_from_bdr() {
        size_t e = boundary.random();
        const Edge& edge = graph.edge(e);
        size_t v = edge.from();
        size_t u = edge.to();
        double diff = 0.0;
        if (!module_vertices.contains(v)) {
            diff += graph.weight(v);
        }
        if (!module_vertices.contains(u)) {
            diff += graph.weight(u);
        }
        diff += edge.weight();
        if (accepts(diff)) {
            add_edge(e);
        }
        score += diff;
    }

    void SimulatedAnnealing::remove_from_module() {
        size_t e = module_edges.random();
        const Edge& edge = graph.edge(e);
        double diff = -edge.weight();
        size_t v = edge.from();
        size_t u = edge.to();
        if (degree[v] == 1 && degree[u] == 1) {
            if (unif() > 0.5) {
                std::swap(v, u);
            }
            diff -= graph.weight(u);
            if (accepts(diff)) {
                if(remove_edge(e, v, u)) {
                    score += diff;
                }
            }
            return;
        }

        if (degree[v] == 1) {
            std::swap(v, u);
        }

        if (degree[u] == 1) {
            diff -= graph.weight(v);
            if (accepts(diff)) {
                if (remove_edge(e, v, u)) {
                    score += diff;
                }
            }
            return;
        }
        if (remove_edge(e, v, u)) {
            score += diff;
        }
    }

    StandardUniformDistribution::StandardUniformDistribution(RandomEngine &re) :re(re) {}

    double StandardUniformDistribution::operator()(){
        return unif(re);
    }
}