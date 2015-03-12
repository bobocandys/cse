/*
 * Copyright 2011 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <algorithm>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;

  // MISSING:
  // If the query is empty, we can just return
  if (query.begin() == query.end())
    return finalresult;

  size_t query_len = query.size();
  size_t query_idx = 0;
  string firstQ = query[query_idx];
  // Check if this query has any associated file
  for (uint32_t i = 0; i < arraylen_; i++) {
    IndexTableReader *indexTbRr = itr_array_[i];
    DocIDTableReader *docIDRr = indexTbRr->LookupWord(firstQ);
    DocTableReader *docTbRr = dtr_array_[i];

    if (docIDRr == NULL) {
      // This query is not found in this index file
      continue;
    }

    list<docid_element_header> docIDList = docIDRr->GetDocIDList();

    for (auto it = docIDList.begin(); it != docIDList.end(); it++) {
      DocID_t docid = it->docid;
      uint32_t rank = it->num_positions;
      string filename;
      if (docTbRr->LookupDocID(docid, &filename)) {
        QueryResult result = {filename, rank};
        finalresult.push_back(result);
      }
    }
    delete docIDRr;
  }

  query_idx++;
  // Loop thru the rest of the query
  while (query_idx < query_len) {
    vector<QueryProcessor::QueryResult> former_list = finalresult;
    string nextq = query[query_idx];;
    // Check if this query has any associated file
    for (uint32_t i = 0; i < arraylen_; i++) {
      DocIDTableReader *docIDRr = itr_array_[i]->LookupWord(nextq);

      if (docIDRr == nullptr) {
        // This query is not found
        continue;
      }

      // There are matches. Iterator thru the current result list
      // Updates the ranks if found any, remove unfound result from
      // the list.

      DocTableReader *docTbRr = dtr_array_[i];

      list<docid_element_header> docIDList = docIDRr->GetDocIDList();

      for (auto itr = docIDList.begin(); itr != docIDList.end(); itr++) {
        DocID_t result_docid = itr->docid;
        uint32_t num_positions = itr->num_positions;

        string query_filename;
        // Lookup the result_filename in the query file list,
        // if found a matching filename, return how many times
        // the query word occur in the file
        docTbRr->LookupDocID(result_docid, &query_filename);
        uint32_t result_count = 0;
        while (result_count < finalresult.size()) {
          string query_docname = finalresult[result_count].document_name;
          if (query_filename.compare(query_docname) == 0) {
            finalresult[result_count].rank += num_positions;
          }
          result_count++;
        }
      }
      delete docIDRr;
    }

    // Check the current list with the former list,
    // remove unchanged items
    for (uint32_t i = 0; i < former_list.size(); i++) {
      for (uint32_t j = 0; j < finalresult.size(); j++) {
        string result_docname = finalresult[j].document_name;
        if (former_list[i].document_name.compare(result_docname) == 0
            && former_list[i].rank == finalresult[j].rank) {
          finalresult.erase(finalresult.begin() + j);
        }
      }
    }
    query_idx++;
  }

  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
