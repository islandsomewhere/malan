% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{meiotic_dist}
\alias{meiotic_dist}
\title{Meiotic distance between two individuals}
\usage{
meiotic_dist(ind1, ind2)
}
\arguments{
\item{ind1}{Individual 1}

\item{ind2}{Individual 2}
}
\value{
Number of meioses between \code{ind1} and \code{ind2} if they are in the same pedigree, else -1.
}
\description{
Get the number of meioses between two individuals.
Note, that pedigrees must first have been inferred by \code{\link[=build_pedigrees]{build_pedigrees()}}.
}
