/**
 class_Individual.cpp
 Purpose: C++ class Individual.
 Details: C++ implementation.
  
 @author Mikkel Meyer Andersen
 */

#include "malan_types.h"

#include <stdexcept>

#include <RcppArmadillo.h> // FIXME: Avoid Rcpp here? Only in api_* files?

/*
==========================================
Individual
==========================================
*/
Individual::Individual(int pid, int generation) {
  m_pid = pid;
  m_generation = generation;
  
  m_children = new std::vector<Individual*>();
}

Individual::~Individual() {
  delete m_children;
}

int Individual::get_pid() const {
  return m_pid;
}

int Individual::get_generation() const {
  return m_generation;
}

void Individual::add_child(Individual* child) {
  m_children->push_back(child);
  child->m_father = this;
}

Individual* Individual::get_father() const {
  return m_father;
}

std::vector<Individual*>* Individual::get_children() const {
  return m_children;
}

int Individual::get_children_count() const {
  return m_children->size();
}

bool Individual::pedigree_is_set() const {
  return (m_pedigree_id != 0);
}

int Individual::get_pedigree_id() const {
  return m_pedigree_id;
}

Pedigree* Individual::get_pedigree() const {
  return m_pedigree;
}

void Individual::unset_pedigree() {
  if (!this->pedigree_is_set()) {
    return;
  }
  
  m_pedigree = nullptr;
  m_pedigree_id = 0;
}

void Individual::set_pedigree_id(int id, Pedigree* ped, int* pedigree_size) {
  if (this->pedigree_is_set()) {
    return;
  }
  
  m_pedigree = ped;
  m_pedigree_id = id;
  *pedigree_size += 1;
  ped->add_member(this);
  
  if (m_father != nullptr) {  
    m_father->set_pedigree_id(id, ped, pedigree_size);
  }
  
  for (auto &child : (*m_children)) {
    ped->add_relation(this, child);
    child->set_pedigree_id(id, ped, pedigree_size);
  }
}

void Individual::dijkstra_reset() {
  m_dijkstra_visited = false;
  m_dijkstra_distance = 0;
}

void Individual::dijkstra_tick_distance(int step) {
  m_dijkstra_distance += step;
}
  
void Individual::dijkstra_set_distance_if_less(int dist) {
  if (m_dijkstra_distance < dist) {
    m_dijkstra_distance = dist;
  }
}

void Individual::dijkstra_mark_visited() {
  m_dijkstra_visited = true;
}

int Individual::dijkstra_get_distance() const {
  return m_dijkstra_distance; 
}

bool Individual::dijkstra_was_visited() const {
  return m_dijkstra_visited; 
}


// Heavily relies on it being a TREE, hence there is only one path connecting every pair of nodes
void Individual::meiosis_dist_tree_internal(Individual* dest, int* dist) const {
  if (this->get_pid() == dest->get_pid()) {
    *dist = dest->dijkstra_get_distance();    
    return;
  }
  
  if (dest->dijkstra_was_visited()) {
    return;
  }
  
  dest->dijkstra_mark_visited();
  dest->dijkstra_tick_distance(1);
  int m = dest->dijkstra_get_distance();
  
  Individual* father = dest->get_father();
  if (father != nullptr) {  
    father->dijkstra_tick_distance(m);    
    this->meiosis_dist_tree_internal(father, dist); 
  }
  
  std::vector<Individual*>* children = dest->get_children();
  for (auto child : *children) {
    child->dijkstra_tick_distance(m);

    this->meiosis_dist_tree_internal(child, dist);
  }
}

// Heavily relies on it being a TREE, hence there is only one path connecting every pair of nodes
int Individual::meiosis_dist_tree(Individual* dest) const {
  if (!(this->pedigree_is_set())) {
    throw std::invalid_argument("!(this->pedigree_is_set())");
  }
  
  if (dest == nullptr) {
    throw std::invalid_argument("dest is NULL");
  }
  
  if (!(dest->pedigree_is_set())) {
    throw std::invalid_argument("!(dest->pedigree_is_set())");
  }
  
  if (this->get_pedigree_id() != dest->get_pedigree_id()) {
    return -1;
  }
  
  // At this point, the individuals this and dest belong to same pedigree
    
  std::vector<Individual*>* inds = this->get_pedigree()->get_all_individuals();
  for (auto child : *inds) {
    child->dijkstra_reset();
  }

  int dist = 0;
  this->meiosis_dist_tree_internal(dest, &dist);
  return dist;
}



/*
Father haplotype
FIXME mutation_model?
*/
void Individual::haplotype_mutate(std::vector<double>& mutation_rates) {
  if (!m_haplotype_set) {
    throw std::invalid_argument("Father haplotype not set yet, so cannot mutate");
  }
  if (m_haplotype.size() != mutation_rates.size()) {
    throw std::invalid_argument("Number of loci specified in haplotype must equal number of mutation rates specified");
  }
  if (m_haplotype_mutated) {
    throw std::invalid_argument("Father haplotype already set and mutated");
  }
  
  
  for (int loc = 0; loc < m_haplotype.size(); ++loc) {
    if (R::runif(0.0, 1.0) < mutation_rates[loc]) {
      if (R::runif(0.0, 1.0) < 0.5) {
        m_haplotype[loc] = m_haplotype[loc] - 1;
      } else {
        m_haplotype[loc] = m_haplotype[loc] + 1;
      }
    }
  }
}


void Individual::haplotype_mutate_ladder_bounded(std::vector<double>& mutation_rates, std::vector<int>& ladder_min, std::vector<int>& ladder_max) {
  if (!m_haplotype_set) {
    throw std::invalid_argument("Father haplotype not set yet, so cannot mutate");
  }
  if (m_haplotype.size() != mutation_rates.size()) {
    throw std::invalid_argument("Number of loci specified in haplotype must equal number of mutation rates specified");
  }
  if (m_haplotype_mutated) {
    throw std::invalid_argument("Father haplotype already set and mutated");
  }  
  
  for (int loc = 0; loc < m_haplotype.size(); ++loc) {
    if (R::runif(0.0, 1.0) < mutation_rates[loc]) {
      // A mutation must happen:
      
      if (m_haplotype[loc] < ladder_min[loc]) {
        Rcpp::Rcout << "Locus (0-based): " << loc << std::endl;
        Rcpp::Rcout << "Haplotype " << m_haplotype[loc] << std::endl;
        Rcpp::Rcout << "Ladder min " << ladder_min[loc] << std::endl;
        Rcpp::print(Rcpp::wrap(m_haplotype));
        Rcpp::print(Rcpp::wrap(ladder_min));
        
        throw std::invalid_argument("Haplotype locus lower than ladder minimum");
      }      
      if (m_haplotype[loc] > ladder_max[loc]) {
        Rcpp::Rcout << "Locus (0-based): " << loc << std::endl;
        Rcpp::Rcout << "Haplotype " << m_haplotype[loc] << std::endl;
        Rcpp::Rcout << "Ladder max " << ladder_max[loc] << std::endl;
        Rcpp::print(Rcpp::wrap(m_haplotype));
        Rcpp::print(Rcpp::wrap(ladder_max));
        
        throw std::invalid_argument("Haplotype locus higher than ladder minimum");
      }

      /*
      if (m_haplotype[loc] <= ladder_min[loc]) {
        // Already at lower bound (or less, by wrong initial conditions), move upwards
        m_haplotype[loc] = ladder_min[loc] + 1; // mutate upwards
      } else if (m_haplotype[loc] >= ladder_max[loc]) {
        // Already at upper bound (or more, by wrong initial conditions), move downwards
        m_haplotype[loc] = ladder_max[loc] - 1;
      }*/
      if (m_haplotype[loc] == ladder_min[loc]) {
        // Already at lower bound, move upwards
        m_haplotype[loc] = ladder_min[loc] + 1; // mutate upwards        
        //Rcpp::Rcout << "Hit lower bound, mutating upwards: " << ladder_min[loc] << " -> " << m_haplotype[loc] << std::endl;
      } else if (m_haplotype[loc] == ladder_max[loc]) {
        // Already at upper bound, move downwards
        m_haplotype[loc] = ladder_max[loc] - 1;
        //Rcpp::Rcout << "Hit upper bound, mutating downwards: " << ladder_max[loc] << " -> " << m_haplotype[loc] << std::endl;
      }
       else {
        // Somewhere on non-boundary ladder, choose direction
        if (R::runif(0.0, 1.0) < 0.5) {
          m_haplotype[loc] = m_haplotype[loc] - 1;
        } else {
          m_haplotype[loc] = m_haplotype[loc] + 1;
        }
      }
    }
  }
}


bool Individual::is_haplotype_set() const {
  return m_haplotype_set; 
}

void Individual::set_haplotype(std::vector<int> h) {
  m_haplotype = h;
  m_haplotype_set = true;
}

std::vector<int> Individual::get_haplotype() const {
  return m_haplotype;
}

void Individual::pass_haplotype_to_children(bool recursive, std::vector<double>& mutation_rates) {
  for (auto &child : (*m_children)) {
    child->set_haplotype(m_haplotype);
    child->haplotype_mutate(mutation_rates);
    
    if (recursive) {
      child->pass_haplotype_to_children(recursive, mutation_rates);
    }
  }
}

void Individual::pass_haplotype_to_children_ladder_bounded(bool recursive, std::vector<double>& mutation_rates, std::vector<int>& ladder_min, std::vector<int>& ladder_max) {
  for (auto &child : (*m_children)) {
    child->set_haplotype(m_haplotype);
    child->haplotype_mutate_ladder_bounded(mutation_rates, ladder_min, ladder_max);
    
    if (recursive) {
      child->pass_haplotype_to_children_ladder_bounded(recursive, mutation_rates, ladder_min, ladder_max);
    }
  }
}

int Individual::get_haplotype_L1(Individual* dest) const {
  std::vector<int> h_this = this->get_haplotype();
  std::vector<int> h_dest = dest->get_haplotype();
  
  if (h_this.size() != h_dest.size()) {
    Rcpp::Rcout << "this pid = " << this->get_pid() << " has haplotype with " << h_this.size() << " loci" << std::endl;
    Rcpp::Rcout << "dest pid = " << dest->get_pid() << " has haplotype with " << h_dest.size() << " loci" << std::endl;
    throw std::invalid_argument("h_this.size() != h_dest.size()");
  }
  
  int d = 0;
  for (size_t i = 0; i < h_this.size(); ++i) {
    d += abs(h_this[i] - h_dest[i]);
  }
  
  return d;
}



std::vector<Individual*> Individual::calculate_path_to(Individual* dest) const {
  if (!(this->pedigree_is_set())) {
    throw std::invalid_argument("!(this->pedigree_is_set())");
  }
  
  if (dest == nullptr) {
    throw std::invalid_argument("dest is NULL");
  }
  
  if (!(dest->pedigree_is_set())) {
    throw std::invalid_argument("!(dest->pedigree_is_set())");
  }
  
  if (this->get_pedigree_id() != dest->get_pedigree_id()) {
    std::vector<Individual*> empty_vec;
    return empty_vec;
  }
  
  // At this point, the individuals this and dest belong to same pedigree
  
  Individual* root = this->get_pedigree()->get_root();
  
  std::vector<Individual*> path_this, path_dest;

  if (!find_path_from_root_to_dest(root, path_this, this)) {
    Rcpp::Rcout << "this pid = " << this->get_pid() << std::endl;  
    throw std::invalid_argument("Could not find path between root and this");
  }
  
  if (!find_path_from_root_to_dest(root, path_dest, dest)) {
    Rcpp::Rcout << "dest pid = " << dest->get_pid() << std::endl;
    throw std::invalid_argument("Could not find path between root and dest");
  }
  
  int LCA_index = 0;
  for (LCA_index = 0; LCA_index < path_this.size() && LCA_index < path_dest.size(); LCA_index++) {
    if (path_this[LCA_index]->get_pid() != path_dest[LCA_index]->get_pid()) {
      break;
    }
  }
  
  if (LCA_index == 0) {
    throw std::invalid_argument("LCA_index cannot be 0");
  }
  
  std::vector<Individual*> path_result;
  path_result.push_back(path_this[LCA_index - 1]); // LCA = path_this[LCA_index - 1] == path_dest[LCA_index - 1]
  path_result.insert(path_result.end(), path_this.begin() + LCA_index, path_this.end());
  path_result.insert(path_result.end(), path_dest.begin() + LCA_index, path_dest.end());
  
  return path_result;
}


int possible_mutate_index(const int index, const double mutation_rate, const int max) {
  if (max <= 0) {
    throw std::invalid_argument("max must be >= 1");
  }
  
  if (R::runif(0.0, 1.0) >= mutation_rate) {
    // No mutation happened
    return index;
  }  

  // A mutation must happen:  
  if (index == 0) {
    return 1;
  }
  
  if (index == max) {
    return max - 1;
  }

  // Somewhere on non-boundary ladder, choose direction
  if (R::runif(0.0, 1.0) < 0.5) {
    return index - 1;
  } else {
    return index + 1;
  }
}

void Individual::pass_autosomal_to_children(bool recursive, 
    const std::vector< std::vector<double> >& allele_conditional_cumdists_theta,
    const double mutation_rate) {

  
  for (auto &child : (*m_children)) {
    /*
    We have theta, so the alleles in the child should be correlated.
    */
    
    /*
    
    // FIXME: Slow, but easy to implement: rejection sampling, ensures theta
    std::vector<int> geno_father = m_haplotype;
    int father_allele = (R::runif(0.0, 1.0) < 0.5) ? geno_father[0] : geno_father[1];
    std::vector<int> geno = draw_autosomal_genotype(allele_cumdist_theta, alleles_count);
    // randomly switch entries
    if (R::runif(0.0, 1.0) < 0.5) {
      int tmp = geno[0];
      geno[0] = geno[1];
      geno[1] = tmp;
    }
    //while (!(geno[0] == father_allele || geno[1] == father_allele)) {
    while (geno[0] != father_allele) {
      geno = draw_autosomal_genotype(allele_cumdist_theta, alleles_count);
      if (R::runif(0.0, 1.0) < 0.5) {
        int tmp = geno[0];
        geno[0] = geno[1];
        geno[1] = tmp;
      }
    }
    */
    
    std::vector<int> geno_father = m_haplotype;
    int father_allele = (R::runif(0.0, 1.0) < 0.5) ? geno_father[0] : geno_father[1];
    std::vector<double> cumdist = allele_conditional_cumdists_theta[father_allele];
    double u = R::runif(0.0, 1.0);
    int alleles_count = cumdist.size();
    int mother_allele = 0;
    
    if (u > cumdist[0]) {
      for (int i = 1; i < alleles_count; ++i) {
        if (u <= cumdist[i]) {
          mother_allele = i;
          break;
        }
      }
    }
    
    std::vector<int> geno(2);
    geno[0] = father_allele;
    geno[1] = mother_allele;
    
    // mutate:
    // m_haplotype has indices of alleles
    int max = alleles_count - 1; // index
    geno[0] = possible_mutate_index(geno[0], mutation_rate, max);
    geno[1] = possible_mutate_index(geno[1], mutation_rate, max);
    
    if (geno[1] <= geno[0]) {
      int tmp = geno[0];
      geno[0] = geno[1];
      geno[1] = tmp;
    }
    
    child->set_haplotype(geno);
    
    if (recursive) {
      child->pass_autosomal_to_children(recursive, allele_conditional_cumdists_theta, mutation_rate);
    }
  }
}

