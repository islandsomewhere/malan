/**
 api_utility_haplotypes.cpp
 Purpose: Logic related to haplotypes.
 Details: API between R user and C++ logic.
  
 @author Mikkel Meyer Andersen
 */

#include <RcppArmadillo.h>

// [[Rcpp::depends(RcppProgress)]]
#include <progress.hpp>

#include <string>

#include "malan_types.hpp"
#include "api_utility_individual.hpp"


//' Populate haplotypes in pedigrees (0-founder/unbounded).
//' 
//' Populate haplotypes from founder and down in all pedigrees.
//' Note, that haplotypes are unbounded and 
//' that all founders get haplotype `rep(0L, loci)`.
//' 
//' @param pedigrees Pedigree list in which to populate haplotypes
//' @param loci Number of loci
//' @param mutation_rates Vector with mutation rates, length `loci`
//' @param progress Show progress
//'
//' @seealso [pedigrees_all_populate_haplotypes_custom_founders()] and 
//' [pedigrees_all_populate_haplotypes_ladder_bounded()].
//' 
//' @export
// [[Rcpp::export]]
void pedigrees_all_populate_haplotypes(Rcpp::XPtr< std::vector<Pedigree*> > pedigrees, 
                                       int loci, 
                                       Rcpp::NumericVector mutation_rates, 
                                       bool progress = true) {
  std::vector<Pedigree*> peds = (*pedigrees);
  
  std::vector<double> mut_rates = Rcpp::as< std::vector<double> >(mutation_rates);
  
  if (loci != mut_rates.size()) {
    Rcpp::stop("Number of loci specified in haplotype must equal number of mutation rates specified");
  }
  
  size_t N = peds.size();
  Progress p(N, progress);
  
  for (size_t i = 0; i < N; ++i) {
    peds.at(i)->populate_haplotypes(loci, mut_rates);
    
     if (i % CHECK_ABORT_EVERY == 0 && Progress::check_abort()) {
      Rcpp::stop("Aborted.");
    }
    
    if (progress) {
      p.increment();
    }
  }
}

//' Populate haplotypes in pedigrees (custom founder/unbounded).
//' 
//' Populate haplotypes from founder and down in all pedigrees.
//' Note, that haplotypes are unbounded.
//' All founders get a haplotype from calling the user 
//' provided function `get_founder_haplotype()`.
//' 
//' @param pedigrees Pedigree list in which to populate haplotypes
//' @param mutation_rates Vector with mutation rates
//' @param get_founder_haplotype Function taking no arguments returning a haplotype of `length(mutation_rates)`
//' @param progress Show progress
//'
//' @seealso [pedigrees_all_populate_haplotypes()] and 
//' [pedigrees_all_populate_haplotypes_ladder_bounded()].
//' 
//' @export
// [[Rcpp::export]]
void pedigrees_all_populate_haplotypes_custom_founders(Rcpp::XPtr< std::vector<Pedigree*> > pedigrees, 
                                       Rcpp::NumericVector mutation_rates,
                                       Rcpp::Nullable<Rcpp::Function> get_founder_haplotype = R_NilValue,
                                       bool progress = true) {
  std::vector<Pedigree*> peds = (*pedigrees);
  
  std::vector<double> mut_rates = Rcpp::as< std::vector<double> >(mutation_rates);
  
  if (get_founder_haplotype.isNull()) {
    Rcpp::stop("get_founder_haplotype must not be NULL");
  }  
  
  Rcpp::Function g_founder_hap = Rcpp::as<Rcpp::Function>(get_founder_haplotype);

  size_t N = peds.size();
  Progress p(N, progress);
  
  for (size_t i = 0; i < N; ++i) {
    peds.at(i)->populate_haplotypes_custom_founders(mut_rates, g_founder_hap);
    
     if (i % CHECK_ABORT_EVERY == 0 && Progress::check_abort()) {
      Rcpp::stop("Aborted.");
    }
    
    if (progress) {
      p.increment();
    }
  }
}

//' Populate haplotypes in pedigrees (custom founder/bounded).
//' 
//' Populate haplotypes from founder and down in all pedigrees.
//' Note, that haplotypes are bounded by `ladder_min` and `ladder_max`.
//' All founders get a haplotype from calling the user 
//' provided function [get_founder_haplotype()].
//' 
//' @param pedigrees Pedigree list in which to populate haplotypes
//' @param mutation_rates Vector with mutation rates
//' @param ladder_min Lower bounds for haplotypes, same length as `mutation_rates`
//' @param ladder_max Upper bounds for haplotypes, same length as `mutation_rates`; all entries must be strictly greater than `ladder_min`
//' @param get_founder_haplotype Function taking no arguments returning a haplotype of `length(mutation_rates)`
//' @param progress Show progress
//'
//' @seealso [pedigrees_all_populate_haplotypes()] and 
//' [pedigrees_all_populate_haplotypes_custom_founders()].
//' 
//' @export
// [[Rcpp::export]]
void pedigrees_all_populate_haplotypes_ladder_bounded(Rcpp::XPtr< std::vector<Pedigree*> > pedigrees, 
                                                      Rcpp::NumericVector mutation_rates, 
                                                      Rcpp::IntegerVector ladder_min,
                                                      Rcpp::IntegerVector ladder_max,
                                                      Rcpp::Nullable<Rcpp::Function> get_founder_haplotype = R_NilValue,
                                                      bool progress = true) {
  //https://stackoverflow.com/questions/36992627/can-rcppfunction-be-null

  if (ladder_min.size() != ladder_max.size()) {
    Rcpp::stop("ladder_min and ladder_max must have same length");
  }
  
  if (any((ladder_max - ladder_min) <= 0).is_true()) {
    Rcpp::stop("ladder_max must be at least 1 greater than ladder_min at all loci");
  }
    
  std::vector<Pedigree*> peds = (*pedigrees);

  std::vector<double> mut_rates = Rcpp::as< std::vector<double> >(mutation_rates);
  std::vector<int> lad_min = Rcpp::as< std::vector<int> >(ladder_min);
  std::vector<int> lad_max = Rcpp::as< std::vector<int> >(ladder_max);
  

  if (mutation_rates.size() != lad_min.size()) {
    Rcpp::stop("mutation_rates and ladder_min must have same length");
  }

  if (mutation_rates.size() != lad_max.size()) {
    Rcpp::stop("mutation_rates and ladder_max must have same length");
  }

  if (get_founder_haplotype.isNull()) {
    Rcpp::stop("get_founder_haplotype must not be NULL");
  }  
  
  Rcpp::Function g_founder_hap = Rcpp::as<Rcpp::Function>(get_founder_haplotype);
    
  size_t N = peds.size();
  Progress p(N, progress);
  
  for (size_t i = 0; i < N; ++i) {
    peds.at(i)->populate_haplotypes_ladder_bounded(mut_rates, lad_min, lad_max, g_founder_hap);
    
     if (i % CHECK_ABORT_EVERY == 0 && Progress::check_abort()) {
      Rcpp::stop("Aborted.");
    }
    
    if (progress) {
      p.increment();
    }
  }
}


//' Get haplotype from an individual
//' 
//' Requires that haplotypes are first populated, e.g. 
//' with [pedigrees_all_populate_haplotypes()], 
//' [pedigrees_all_populate_haplotypes_custom_founders()], or 
//' [pedigrees_all_populate_haplotypes_ladder_bounded()].
//' 
//' @param individual Individual to get haplotypes for.
//' @return Haplotype for `individual`.
//' 
//' @seealso [get_haplotypes_individuals()] and [get_haplotypes_pids()].
//' 
//' @export
// [[Rcpp::export]]
std::vector<int> get_haplotype(Rcpp::XPtr<Individual> individual) {
  if (!individual->is_haplotype_set()) {
    Rcpp::stop("Haplotype not yet set.");
  }
  
  return individual->get_haplotype();
}



//' Get haplotype matrix from list of individuals
//' 
//' Requires that haplotypes are first populated, e.g. 
//' with [pedigrees_all_populate_haplotypes()], 
//' [pedigrees_all_populate_haplotypes_custom_founders()], or 
//' [pedigrees_all_populate_haplotypes_ladder_bounded()].
//' 
//' @param individuals Individuals to get haplotypes for.
//' @return Matrix of haplotypes where row `i` is the haplotype of `individuals[[i]]`.
//' 
//' @seealso [get_haplotypes_pids()].
//' 
//' @export
// [[Rcpp::export]]
Rcpp::IntegerMatrix get_haplotypes_individuals(Rcpp::ListOf< Rcpp::XPtr<Individual> > individuals) {   
  size_t n = individuals.size();
  
  if (n <= 0) {
    Rcpp::IntegerMatrix empty_haps(0, 0);
    return empty_haps;
  }
  
  size_t loci = individuals[0]->get_haplotype().size();
  
  if (loci <= 0) {
    Rcpp::stop("Expected > 0 loci");
    Rcpp::IntegerMatrix empty_haps(0, 0);
    return empty_haps;
  }
  
  Rcpp::IntegerMatrix haps(n, loci);
  
  for (size_t i = 0; i < n; ++i) {
    Individual* individual = individuals[i];
    
    if (!individual->is_haplotype_set()) {
      Rcpp::stop("Haplotype not yet set.");
    }
    
    std::vector<int> hap = individual->get_haplotype();
    
    if (hap.size() != loci) {
      Rcpp::stop("Expected > 0 loci for all haplotypes");
      Rcpp::IntegerMatrix empty_haps(0, 0);
      return empty_haps;
    }
    
    Rcpp::IntegerVector h = Rcpp::wrap(hap);
    haps(i, Rcpp::_) = h;
  }
  
  return haps;
}

//' Get haplotypes from a vector of pids.
//' 
//' Requires that haplotypes are first populated, e.g. 
//' with [pedigrees_all_populate_haplotypes()], 
//' [pedigrees_all_populate_haplotypes_custom_founders()], or 
//' [pedigrees_all_populate_haplotypes_ladder_bounded()].
//' 
//' @param population Population
//' @param pids Vector of pids to get haplotypes for.
//' 
//' @return Matrix of haplotypes where row `i` is the haplotype of `individuals[[i]]`.
//' 
//' @seealso [get_haplotypes_individuals()].
//' 
//' @export
// [[Rcpp::export]]
Rcpp::IntegerMatrix get_haplotypes_pids(Rcpp::XPtr<Population> population, Rcpp::IntegerVector pids) {
  size_t n = pids.size();
  
  if (n <= 0) {
    Rcpp::IntegerMatrix empty_haps(0, 0);
    return empty_haps;
  }
  
  Individual* ind = population->get_individual(pids[0]);
  
  if (!ind->is_haplotype_set()) {
    Rcpp::stop("Haplotype not yet set.");
  }
  
  std::vector<int> hap = ind->get_haplotype();
  size_t loci = hap.size();
  
  if (loci <= 0) {
    Rcpp::stop("Expected > 0 loci");
    Rcpp::IntegerMatrix empty_haps(0, 0);
    return empty_haps;
  }
  
  Rcpp::IntegerMatrix haps(n, loci);
  
  Rcpp::IntegerVector h = Rcpp::wrap(hap);
  haps(0, Rcpp::_) = h;
  
  // i = 0 already taken above
  for (size_t i = 1; i < n; ++i) {
    ind = population->get_individual(pids[i]);
    
    if (!ind->is_haplotype_set()) {
      Rcpp::stop("Haplotype not yet set.");
    }
    
    hap = ind->get_haplotype();
    
    if (hap.size() != loci) {
      Rcpp::stop("Expected > 0 loci for all haplotypes");
      Rcpp::IntegerMatrix empty_haps(0, 0);
      return empty_haps;
    }
    
    h = Rcpp::wrap(hap);
    haps(i, Rcpp::_) = h;
  }
  
  return haps;
}

//' Count haplotypes occurrences in list of individuals
//' 
//' Counts the number of types `haplotype` appears in `individuals`.
//' 
//' @param individuals List of individuals to count occurrences in.
//' @param haplotype Haplotype to count occurrences of.
//' 
//' @return Number of times that `haplotype` occurred amongst `individuals`.
//' 
//' @seealso [pedigree_haplotype_matches_in_pedigree_meiosis_L1_dists()].
//' 
//' @export
// [[Rcpp::export]]
int count_haplotype_occurrences_individuals(const Rcpp::List individuals, const Rcpp::IntegerVector haplotype) {
  int n = individuals.size();
  int loci = haplotype.size();
  int count = 0;
  
  std::vector<int> h = Rcpp::as< std::vector<int> >(haplotype);
  
  for (int i = 0; i < n; ++i) {
    Rcpp::XPtr<Individual> indv = individuals[i];
    
    if (!indv->is_haplotype_set()) {
      Rcpp::stop("Haplotype not yet set.");
    }
    
    std::vector<int> indv_h = indv->get_haplotype();
    
    if (indv_h.size() != loci) {
      Rcpp::stop("haplotype and indv_h did not have same number of loci");
    }
    
    if (indv_h == h) {
      count += 1;
    }
  }
  
  return count;
}


//' Information about matching individuals
//' 
//' Gives information about all individuals in pedigree that matches an individual.
//' Just as [count_haplotype_occurrences_individuals()] counts the number of 
//' occurrences amongst a list of individuals, 
//' this gives detailed information about matching individuals in the pedigree, 
//' e.g. meiotic distances and maximum L1 distance on the path as some of these 
//' matches may have (back)mutations between in between them (but often this will be 0).
//' 
//' @param suspect Individual that others must match the profile of.
//' @param generation_upper_bound_in_result Only consider matches in 
//' generation 0, 1, ... generation_upper_bound_in_result.
//' -1 means disabled, consider all generations.
//' End generation is generation 0.
//' Second last generation is 1. 
//' And so on.
//' 
//' @return Matrix with information about matching individuals. 
//' Columns in order: meioses (meiotic distance to `suspect`), 
//' max_L1 (on the path between the matching individual and `suspect`, 
//' what is the maximum L1 distance between the `suspect`'s profile and the 
//' profiles of the individuals on the path), 
//' pid (pid of matching individual)
//' 
//' @seealso [count_haplotype_occurrences_individuals()].
//'
//' @export
// [[Rcpp::export]]
Rcpp::IntegerMatrix pedigree_haplotype_matches_in_pedigree_meiosis_L1_dists(const Rcpp::XPtr<Individual> suspect, 
                                                                            int generation_upper_bound_in_result = -1) {
  const std::vector<int> h = suspect->get_haplotype();

  const Pedigree* pedigree = suspect->get_pedigree();
  const int suspect_pedigree_id = suspect->get_pedigree_id();
  const std::vector<Individual*>* family = pedigree->get_all_individuals();
  
  std::vector<int> meiosis_dists;
  std::vector<int> max_L1_dists;
  std::vector<int> pids;
  
  // includes suspect by purpose
  for (auto dest : *family) { 
    int generation = dest->get_generation();
    
    if (generation_upper_bound_in_result != -1 && generation > generation_upper_bound_in_result) {
      continue;
    }
    
    // only considering within pedigree matches
    if (dest->get_pedigree_id() != suspect_pedigree_id) {
      continue;
    }
    
    std::vector<int> dest_h = dest->get_haplotype();
    
    if (dest_h.size() != h.size()) {
      Rcpp::stop("haplotype and dest_h did not have same number of loci");
    }
    
    if (dest_h == h) {
      std::vector<Individual*> path = suspect->calculate_path_to(dest);  
      int meiosis_dist = suspect->meiosis_dist_tree(dest);
      
      int meiosis_dist_from_path = path.size() - 1; // n vertices means n-1 edges (tree)
      //Rcpp::Rcout << ">> path from " << suspect->get_pid() << " to " << dest->get_pid() << " has length = " << meiosis_dist_from_path << " and meioses = " << meiosis_dist << (meiosis_dist_from_path == meiosis_dist ? " ok" : " ERROR") << ": " << std::endl;
      
      int max_L1 = 0;
      
      //Rcpp::Rcout << "  ";
      
      for (auto intermediate_node : path) { 
        //Rcpp::Rcout << intermediate_node->get_pid();
        
        int d = suspect->get_haplotype_L1(intermediate_node);
        
        if (d > max_L1) {
          max_L1 = d;
          //Rcpp::Rcout << "!";
        }
        
        //Rcpp::Rcout << " ";
      }
      
      //Rcpp::Rcout << std::endl;      
      
      if (meiosis_dist == -1) {
        Rcpp::stop("Cannot occur in pedigree!");
      }
      
      meiosis_dists.push_back(meiosis_dist);
      max_L1_dists.push_back(max_L1);
      pids.push_back(dest->get_pid());
    }
  }
  
  size_t n = meiosis_dists.size();
  
  Rcpp::IntegerMatrix matches(n, 3);
  colnames(matches) = Rcpp::CharacterVector::create("meioses", "max_L1", "pid");
  
  for (size_t i = 0; i < n; ++i) {
    matches(i, 0) = meiosis_dists[i];
    matches(i, 1) = max_L1_dists[i];
    matches(i, 2) = pids[i];
  }
  
  return matches;
}
  
//' @export
// [[Rcpp::export]]
int meiotic_dist(Rcpp::XPtr<Individual> ind1, Rcpp::XPtr<Individual> ind2) {
  return ind1->meiosis_dist_tree(ind2);
}

//' @export
// [[Rcpp::export]]
int count_haplotype_occurrences_pedigree(Rcpp::XPtr<Pedigree> pedigree, const Rcpp::IntegerVector haplotype, int generation_upper_bound_in_result = -1) {
  int loci = haplotype.size();
  int count = 0;
  
  std::vector<int> h = Rcpp::as< std::vector<int> >(haplotype);

  std::vector<Individual*>* family = pedigree->get_all_individuals();

  for (auto dest : *family) {    
    int generation = dest->get_generation();
    
    if (generation_upper_bound_in_result != -1 && generation > generation_upper_bound_in_result) {
      continue;
    }
    
    std::vector<int> dest_h = dest->get_haplotype();
    
    if (dest_h.size() != loci) {
      Rcpp::stop("haplotype and indv_h did not have same number of loci");
    }
    
    if (dest_h == h) {
      count += 1;
    }    
  }
  
  return count;
}

