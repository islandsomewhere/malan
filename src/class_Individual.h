/**
 class_Individual.c
 Purpose: Header for C++ class Individual.
 Details: C++ header.
  
 @author Mikkel Meyer Andersen
 */

#include "malan_types.h"

#include <vector>

class Individual {
private:
  int m_pid; 
  int m_generation = -1;
  
  std::vector<Individual*>* m_children = nullptr;
  Individual* m_father = nullptr;
  
  Pedigree* m_pedigree = nullptr;
  int m_pedigree_id = 0;
  
  void meiosis_dist_tree_internal(Individual* dest, int* dist) const;
  
  bool m_dijkstra_visited = false;
  int m_dijkstra_distance = 0;

  std::vector<int> m_haplotype; // called haplotype, but is used without order for autosomal (as index of alleles)
  bool m_haplotype_set = false;
  bool m_haplotype_mutated = false;
  void haplotype_mutate(std::vector<double>& mutation_rates);
  void haplotype_mutate_ladder_bounded(std::vector<double>& mutation_rates, std::vector<int>& ladder_min, std::vector<int>& ladder_max);
  
public:
  Individual(int pid, int generation);
  ~Individual();
  int get_pid() const;
  int get_generation() const;
  void add_child(Individual* child);
  //void set_father(Individual* i);
  Individual* get_father() const;
  std::vector<Individual*>* get_children() const;
  int get_children_count() const;
  bool pedigree_is_set() const;
  Pedigree* get_pedigree() const;
  int get_pedigree_id() const;
  
  void set_pedigree_id(int id, Pedigree* ped, int* pedigree_size);

  int meiosis_dist_tree(Individual* dest) const;
  
  std::vector<Individual*> calculate_path_to(Individual* dest) const;
  
  void dijkstra_reset();
  void dijkstra_tick_distance(int step);
  void dijkstra_set_distance_if_less(int dist);
  void dijkstra_mark_visited();
  int dijkstra_get_distance() const;
  bool dijkstra_was_visited() const;
  
  bool is_haplotype_set() const;
  void set_haplotype(std::vector<int> h);
  std::vector<int> get_haplotype() const;
  void pass_haplotype_to_children(bool recursive, std::vector<double>& mutation_rates);
  void pass_haplotype_to_children_ladder_bounded(bool recursive, std::vector<double>& mutation_rates, std::vector<int>& ladder_min, std::vector<int>& ladder_max);
  
  int get_haplotype_L1(Individual* dest) const;
  
  void pass_autosomal_to_children(bool recursive, 
    const std::vector<double>& allele_cumdist_theta,
    const int alleles_count,
    const double mutation_rate);
};

