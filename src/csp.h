#include <initializer_list>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>

template <typename V, typename D>
class Constraint 
{
    public:
        std::unordered_set<V> variables;
        virtual bool satisfied(const std::unordered_map<V, D>& assignment) const = 0;
};

template <typename V, typename D>
class CSPSolver {

    public:
        std::unordered_set<V> variables;
        std::unordered_map<V, std::unordered_set<D>> domains;
        std::unordered_map<V, std::vector<std::reference_wrapper<Constraint<V, D>>>> constraints;

        CSPSolver(std::unordered_set<V>& _variables, std::unordered_map<V, std::unordered_set<D>>& _domains)
        {
            variables = _variables;
            domains = _domains;
        };

        void add_constraint(Constraint<V, D>& constraint) 
        {
            for (auto& variable: constraint.variables)
                if (variables.find(variable) != variables.end())
                    constraints[variable].push_back(constraint);
                else
                    throw std::domain_error("variable not in CSP.");
        };

        bool consistent(V& variable, std::unordered_map<V, D>& assignment)
        {
            for (auto& constraint: constraints[variable])
                if (!constraint.get().satisfied(assignment))
                    return false;
            return true; 
        };

        std::unordered_map<V, D> backtracking_search(
                std::unordered_map<V, D> assignment, 
                std::function<bool(void)> force_stop = [](){ return false; }
        ){
           if (assignment.size() == variables.size())
               return assignment;

            std::vector<V> unassigned;
            for (auto& variable: variables)
                if (assignment.find(variable) == assignment.end())
                    unassigned.push_back(variable);

           
            auto variable = unassigned[0];
            for (auto& value: domains[variable]) 
            {
                if (force_stop()) return assignment;

                auto local_assignment = assignment;
                local_assignment[variable] = value;
                if (consistent(variable, local_assignment))
                {
                    auto result = backtracking_search(local_assignment, std::ref(force_stop));
                    if (result.size() != 0)
                        return result;
                }
            }
            
            return {};
        };
};

inline auto CreateVariables(std::string name, int from, int to)
{
    std::unordered_set<std::string> variables;
    for (int i = from; i < to; ++i) { variables.insert(name + std::to_string(i)); }
    return variables;
};

inline auto CreateVariables(std::initializer_list<std::string> names)
{
    std::unordered_set<std::string> variables;
    for (auto name: names) { variables.insert(name); };
    return variables;
}

inline auto JoinVariables(std::unordered_set<std::string> vars0, std::unordered_set<std::string> vars1)
{
    std::unordered_set<std::string> variables;
    for (auto var: vars0) { variables.insert(var); }
    for (auto var: vars1) { variables.insert(var); }
    return variables;
}

inline auto Domain(int from, int to, int step)
{
    std::unordered_set<int> domain;
    for (int i = from; i < to; i += step){ domain.insert(i); }
    return domain;
}

template <typename V>
void Zip(std::unordered_set<V> v0, std::unordered_set<V> v1, std::function<void(V, V)> func)
{
    auto it0 = v0.begin();
    auto it1 = v1.begin();
    
    while (it0 != v0.end() || it1 != v1.end())
    {
        func(*it0, *it1);
        it0 = std::next(it0);
        it1 = std::next(it1);
    }
};

template <typename V>
void ForEach(std::unordered_set<V> v0, std::unordered_set<V> v1, std::function<void(V, V)> func)
{
    auto it0 = v0.begin();
    while (it0 != v0.end())
    {
        auto it1 = v1.begin();
        while (it1 != v1.end())
        {
            if (*it0 != *it1)
            {
                func(*it0, *it1);
            }
            it1 = std::next(it1);
        }
        it0 = std::next(it0);
    }
};

template <typename V>
void Between(std::unordered_set<V> v, std::function<void(V, V)> func)
{
    auto it0 = v.begin();
    while (it0 != v.end())
    {
        auto it1 = std::next(it0);
        while (it1 != v.end())
        {
            func(*it0, *it1);
            it1 = std::next(it1);
        }
        it0 = std::next(it0);
    }
};
