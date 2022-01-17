#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <stdexcept>

template <typename V, typename D>
class Constraint 
{
    public:
        std::unordered_set<V> variables;
        virtual bool satisfied(std::unordered_map<V, D>& assignment) const = 0;
};

template <typename V, typename D>
class CSPSolver {

    public:
        std::unordered_set<V> variables;
        std::unordered_map<V, std::unordered_set<D>> domains;
        std::unordered_map<V, std::unordered_set<Constraint<V, D>>> constraints;

        CSPSolver(std::unordered_set<V>& _variables, std::unordered_map<V, D>& _domains)
        {
            variables = _variables;
            domains = _domains;
        };

        void add_constraint(Constraint<V, D>& constraint) 
        {
            for (auto& variable: constraint.variables)
                if (variables.find(variable) != variables.end())
                    constraints[variable] = constraint;
                else
                    throw std::domain_error("variable not in CSP.");
        };

        bool consistent(V& variable, std::unordered_map<V, D> assignment)
        {
            for (auto& constraint: constraints[variable])
                if (!constraint.satisfied(assignment))
                    return false;
            return true; 
        };

        std::unordered_map<V, D> backtracking_search(std::unordered_map<V, D> assignment)
        {
           if (assignment.size() == variables.size())
               return assignment;

            std::vector<V> unassigned;
            for (auto& variable: variables)
                if (assignment.find(variable) == assignment.end())
                    unassigned.push_back(variable);

            // TODO heuristics
            auto first = unassigned[0];
            for (auto& value: domains[first]) // TODO domain order heuristics
            {
                auto local_assignment = assignment;
                local_assignment[first] = value;

                if (consistent(first, local_assignment))
                    return backtracking_search(local_assignment);

            }
            throw std::logic_error("No solution available.");
        };
};
