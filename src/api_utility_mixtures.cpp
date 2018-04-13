/**
 api_utility_mixtures.cpp
 Purpose: Logic related to mixtures.
 Details: API between R user and C++ logic.
  
 @author Mikkel Meyer Andersen
 */

#include <RcppArmadillo.h>

// [[Rcpp::depends(RcppProgress)]]
#include <progress.hpp>

#include <string>

#include "malan_types.hpp"
#include "api_utility_individual.hpp"

//' Mixture information about 2 persons' mixture of donor1 and donor2.
//' 
//' @param individuals Individuals to consider as possible contributors and thereby get information from.
//' @param donor1 Contributor1/donor 1
//' @param donor2 Contributor2/donor 2
//' @return A list with mixture information about the mixture \code{donor1}+\code{donor2}+\code{donor3} from \code{individuals}
//' 
//' @seealso \code{\link{mixture_info_by_individuals_3pers}}
//' 
//' @export
// [[Rcpp::export]]
Rcpp::List mixture_info_by_individuals(const Rcpp::List individuals, Rcpp::XPtr<Individual>& donor1, Rcpp::XPtr<Individual>& donor2) { 
  size_t N = individuals.size();
  
  Rcpp::List res;
  
  if (N == 0) {
    return res;
  }

  // mainly count wanted, but indices are good for debugging
  Rcpp::IntegerVector res_comp_with_mixture;
  Rcpp::List res_comp_with_mixture_dists;
  Rcpp::IntegerVector res_match_donor1;
  Rcpp::IntegerVector res_match_donor2;
  Rcpp::IntegerVector res_others_included;
  
  std::vector<int> H1 = donor1->get_haplotype();
  std::vector<int> H2 = donor2->get_haplotype();
  
  size_t loci = H1.size();
  
  if (H2.size() != loci) {
    Rcpp::stop("H2.size() != H1.size()");
  }
  
  size_t loci_not_matching = 0;
  
  for (size_t locus = 0; locus < loci; ++locus) {
    if (H1[locus] != H2[locus]) {
      loci_not_matching += 1;
    }
  }

  for (size_t i = 0; i < N; ++i) {
    Rcpp::XPtr<Individual> indv = individuals[i];
    std::vector<int> indv_h = indv->get_haplotype();
    
    if (indv_h.size() != loci) {
      Rcpp::stop("indv_h.size() != H1.size()");
    }
    
    bool in_mixture = true;
    bool match_H1 = true; // faster than Rcpp equal/all sugar 
    bool match_H2 = true;
    
    for (size_t locus = 0; locus < loci; ++locus) {
      if (in_mixture && (indv_h[locus] != H1[locus]) && (indv_h[locus] != H2[locus])) {
        in_mixture = false;
      }
      
      if (match_H1 && (indv_h[locus] != H1[locus])) {
        match_H1 = false;
      }
      
      if (match_H2 && (indv_h[locus] != H2[locus])) {
        match_H2 = false;
      }
      
      // if neither have a chance, just stop
      if (!in_mixture && !match_H1 && !match_H2) {
        break;
      }
    }
    
    int pid = indv->get_pid();
    
    if (in_mixture) {
      res_comp_with_mixture.push_back(pid); // R indexing
      
      //int dist_donor1 = donor1->calculate_path_to(indv);
      //int dist_donor2 = donor2->calculate_path_to(indv);      
      int dist_donor1 = donor1->meiosis_dist_tree(indv);
      int dist_donor2 = donor2->meiosis_dist_tree(indv);
      
      Rcpp::List r = Rcpp::List::create(
        Rcpp::Named("indv_pid") = pid,
        //Rcpp::Named("pid_donor1") = donor1->get_pid(),
        //Rcpp::Named("pid_donor2") = donor2->get_pid(),
        Rcpp::Named("dist_donor1") = dist_donor1,
        Rcpp::Named("dist_donor2") = dist_donor2);   
        
      res_comp_with_mixture_dists.push_back(r);
      
      if (match_H1) {
        res_match_donor1.push_back(pid);
      }
      
      if (match_H2) {
        res_match_donor2.push_back(pid);
      }
      
      if (!match_H1 && !match_H2) {
        res_others_included.push_back(pid);
      }
    }    
  }
  
  res["pids_included_in_mixture"] = res_comp_with_mixture;
  res["pids_included_in_mixture_info"] = res_comp_with_mixture_dists;
  res["pids_matching_donor1"] = res_match_donor1;
  res["pids_matching_donor2"] = res_match_donor2;
  res["pids_others_included"] = res_others_included;
  res["pids_donor12_meiotic_dist"] = donor1->meiosis_dist_tree(donor2);
  res["donor1_family_info"] = get_family_info(donor1);
  res["donor2_family_info"] = get_family_info(donor2);
  res["donor1_profile"] = H1;
  res["donor2_profile"] = H2;
  res["donor1_pid"] = donor1->get_pid();
  res["donor2_pid"] = donor2->get_pid();
  res["loci_not_matching"] = loci_not_matching;

  return res;
}













//' Mixture information about 3 persons' mixture of donor1, donor2 and donor3.
//' 
//' @inherit mixture_info_by_individuals
//' @param donor3 Contributor2/donor 3
//' 
//' @seealso \code{\link{mixture_info_by_individuals}}
//' 
//' @export
// [[Rcpp::export]]
Rcpp::List mixture_info_by_individuals_3pers(const Rcpp::List individuals, 
    Rcpp::XPtr<Individual>& donor1, 
    Rcpp::XPtr<Individual>& donor2, 
    Rcpp::XPtr<Individual>& donor3) { 
    
  size_t N = individuals.size();
  
  Rcpp::List res;
  
  if (N == 0) {
    return res;
  }

  // mainly count wanted, but indices are good for debugging
  Rcpp::IntegerVector res_comp_with_mixture;
  Rcpp::IntegerVector res_match_donor1;
  Rcpp::IntegerVector res_match_donor2;
  Rcpp::IntegerVector res_match_donor3;
  Rcpp::IntegerVector res_others_included;
  
  std::vector<int> H1 = donor1->get_haplotype();
  std::vector<int> H2 = donor2->get_haplotype();
  std::vector<int> H3 = donor3->get_haplotype();
  
  size_t loci = H1.size();
  
  if (H2.size() != loci) {
    Rcpp::stop("H2.size() != H1.size()");
  }

  if (H3.size() != loci) {
    Rcpp::stop("H3.size() != H1.size()");
  }

  for (size_t i = 0; i < N; ++i) {
    Rcpp::XPtr<Individual> indv = individuals[i];
    std::vector<int> indv_h = indv->get_haplotype();
    
    if (indv_h.size() != loci) {
      Rcpp::stop("indv_h.size() != H1.size()");
    }
    
    bool in_mixture = true;
    bool match_H1 = true; // faster than Rcpp equal/all sugar 
    bool match_H2 = true;
    bool match_H3 = true;
    
    for (size_t locus = 0; locus < loci; ++locus) {
      if (in_mixture && 
            (indv_h[locus] != H1[locus]) && 
            (indv_h[locus] != H2[locus]) && 
            (indv_h[locus] != H3[locus])) {
        in_mixture = false;
      }
      
      if (match_H1 && (indv_h[locus] != H1[locus])) {
        match_H1 = false;
      }
      
      if (match_H2 && (indv_h[locus] != H2[locus])) {
        match_H2 = false;
      }
      
      if (match_H3 && (indv_h[locus] != H3[locus])) {
        match_H3 = false;
      }
      
      // if neither have a chance, just stop
      if (!in_mixture && !match_H1 && !match_H2 && !match_H3) {
        break;
      }
    }
    
    int pid = indv->get_pid();
    
    if (in_mixture) {
      res_comp_with_mixture.push_back(pid); // R indexing
            
      if (match_H1) {
        res_match_donor1.push_back(pid);
      }
      
      if (match_H2) {
        res_match_donor2.push_back(pid);
      }
      
      if (match_H3) {
        res_match_donor3.push_back(pid);
      }
      
      if (!match_H1 && !match_H2 && !match_H3) {
        res_others_included.push_back(pid);
      }
    }    
  }
  
  res["pids_included_in_mixture"] = res_comp_with_mixture;
  res["pids_matching_donor1"] = res_match_donor1;
  res["pids_matching_donor2"] = res_match_donor2;
  res["pids_matching_donor3"] = res_match_donor3;
  res["pids_others_included"] = res_others_included;
  res["donor1_profile"] = H1;
  res["donor2_profile"] = H2;
  res["donor3_profile"] = H3;
  res["donor1_pid"] = donor1->get_pid();
  res["donor2_pid"] = donor2->get_pid();
  res["donor3_pid"] = donor3->get_pid();

  return res;
}


