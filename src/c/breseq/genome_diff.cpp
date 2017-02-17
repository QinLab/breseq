/*****************************************************************************

AUTHORS

  Jeffrey E. Barrick <jeffrey.e.barrick@gmail.com>
  David B. Knoester

LICENSE AND COPYRIGHT

  Copyright (c) 2008-2010 Michigan State University
  Copyright (c) 2011-2012 The University of Texas at Austin

  breseq is free software; you can redistribute it and/or modify it under the  
  terms the GNU General Public License as published by the Free Software 
  Foundation; either version 1, or (at your option) any later version.

*****************************************************************************/

#include "libbreseq/genome_diff.h"

#include "libbreseq/flagged_regions.h"
#include "libbreseq/reference_sequence.h"
#include "libbreseq/candidate_junctions.h"
#include "libbreseq/mutation_predictor.h"
#include "libbreseq/output.h"

namespace breseq {

// Common keywords used for diff entries:

// Shared   
const char* SEQ_ID="seq_id";
const char* START="start";
const char* END="end";
const char* STRAND="strand";
const char* POSITION="position";
const char* INSERT_POSITION="insert_position";
const char* PHYLOGENY_ID="phylogeny_id";
const char* FREQUENCY="frequency";
const char* REJECT="reject";
const char* POLYMORPHISM_REJECT="polymorphism_reject";
const char* MEDIATED="mediated";
const char* BETWEEN="between";
  
// For APPLY
const char* WITHIN="within";
const char* BEFORE="before";
const char* APPLY_SIZE_ADJUST = "apply_size_adjust";
  
// For DEL
const char* SIZE="size";

// For INS
const char* NEW_SEQ="new_seq";

// For MOB
const char* REPEAT_NAME = "repeat_name";
const char* DUPLICATION_SIZE = "duplication_size";
const char* INS_START = "ins_start";
const char* INS_END = "ins_end";
const char* DEL_START = "del_start";
const char* DEL_END = "del_end";
const char* MOB_REGION = "mob_region";
  
// For INS/DEL
const char* REPEAT_SEQ = "repeat_seq";
const char* REPEAT_LENGTH = "repeat_length";
const char* REPEAT_REF_COPIES = "repeat_ref_copies";
const char* REPEAT_NEW_COPIES = "repeat_new_copies";
  
// For AMP
const char* NEW_COPY_NUMBER = "new_copy_number";
const char* MEDIATED_STRAND = "mediated_strand";
  
// For CON
const char* REGION = "region";  
  
//For RA
// old + new required field
const char* REF_BASE="ref_base";
// old fields to maintain RA definition
const char* NEW_BASE="new_base";
const char* REF_COV="ref_cov";
const char* NEW_COV="new_cov";
  
// new fields
const char* MAJOR_BASE="major_base";
const char* MINOR_BASE="minor_base";
const char* MAJOR_COV="major_cov";
const char* MINOR_COV="minor_cov";
const char* TOTAL_COV="total_cov";
const char* PREDICTION = "prediction";
const char* SCORE="score";
const char* CONSENSUS_SCORE="consensus_score";
const char* POLYMORPHISM_SCORE="polymorphism_score";
const char* POLYMORPHISM_FREQUENCY="polymorphism_frequency";
const char* MAJOR_FREQUENCY="major_frequency";            // frequency of major allele
const char* POLYMORPHISM_EXISTS="polymorphism_exists";    // internal flag for running R script
  
//For MC  
const char* START_RANGE="start_range";
const char* END_RANGE="end_range";
const char* LEFT_OUTSIDE_COV = "left_outside_cov";
const char* LEFT_INSIDE_COV = "left_inside_cov";
const char* RIGHT_INSIDE_COV = "right_inside_cov";
const char* RIGHT_OUTSIDE_COV = "right_outside_cov";
  
//For JC
const char* SIDE_1_SEQ_ID = "side_1_seq_id";
const char* SIDE_1_POSITION = "side_1_position";
const char* SIDE_1_STRAND = "side_1_strand";
const char* SIDE_1_REDUNDANT = "side_1_redundant";
  
const char* SIDE_2_SEQ_ID = "side_2_seq_id";
const char* SIDE_2_POSITION = "side_2_position";
const char* SIDE_2_STRAND = "side_2_strand";
const char* SIDE_2_REDUNDANT = "side_2_redundant";

const char* OVERLAP = "overlap";
const char* UNIQUE_READ_SEQUENCE = "unique_read_sequence";
  
const char* SIDE_1_READ_COUNT="side_1_read_count";
const char* SIDE_2_READ_COUNT="side_2_read_count";
const char* NEW_JUNCTION_READ_COUNT="new_junction_read_count";
const char* NEW_JUNCTION_FREQUENCY = "new_junction_frequency";

const char* SIDE_1_COVERAGE = "side_1_coverage";
const char* SIDE_2_COVERAGE = "side_2_coverage";
const char* NEW_JUNCTION_COVERAGE = "new_junction_coverage";

//For CN
const char* COPY_NUMBER = "copy_number";
  
const int32_t k_num_line_specification_common_prefix_columns = 3;
  
const map<gd_entry_type, vector<string> > line_specification = make_map<gd_entry_type, vector<string> >
//! seq_id and positions are already parameters in cDiffEntry
//## mutations
(SNP,make_vector<string> (SEQ_ID)(POSITION)(NEW_SEQ))
(SUB,make_vector<string> (SEQ_ID)(POSITION)(SIZE)(NEW_SEQ))
(DEL,make_vector<string> (SEQ_ID)(POSITION)(SIZE))
(INS,make_vector<string> (SEQ_ID)(POSITION)(NEW_SEQ))
(MOB,make_vector<string> (SEQ_ID)(POSITION)(REPEAT_NAME)(STRAND)(DUPLICATION_SIZE))
(INV,make_vector<string> (SEQ_ID)(POSITION)(SIZE))
(AMP,make_vector<string> (SEQ_ID)(POSITION)(SIZE)(NEW_COPY_NUMBER))
(CON,make_vector<string> (SEQ_ID)(POSITION)(SIZE)(REGION))

//## evidence
(RA,make_vector<string> (SEQ_ID)(POSITION)(INSERT_POSITION)(REF_BASE)(NEW_BASE))
(MC,make_vector<string> (SEQ_ID)(START)(END)(START_RANGE)(END_RANGE))
(JC,make_vector<string> (SIDE_1_SEQ_ID)(SIDE_1_POSITION)(SIDE_1_STRAND)(SIDE_2_SEQ_ID)(SIDE_2_POSITION)(SIDE_2_STRAND)(OVERLAP))
(CN,make_vector<string> (SEQ_ID)(START)(END)(COPY_NUMBER))
(UN,make_vector<string> (SEQ_ID)(START)(END))

//## validation
(CURA,make_vector<string> ("expert"))
(FPOS,make_vector<string> ("expert"))
(PHYL,make_vector<string> ("gd"))
(TSEQ,make_vector<string> ("seq_id")("primer_1_start")("primer_1_end")("primer_2_start")("primer_2_end"))
(PFLP,make_vector<string> ("seq_id")("primer_1_start")("primer_1_end")("primer_2_start")("primer_2_end"))
(RFLP,make_vector<string> ("seq_id")("primer_1_start")("primer_1_end")("primer_2_start")("primer_2_end"))
(PFGE,make_vector<string> ("seq_id")("enzyme"))
(NOTE,make_vector<string> ("note"))
(MASK,make_vector<string> (SEQ_ID)(POSITION)(SIZE))

; // end line specifications

// These specs include addition fields used when determined equal mutations and sorting!
// IMPORTANT: They include fields that may not always be defined.
// NOTE: 'unique' gets added to ALL specs for determining uniqueness
const map<gd_entry_type, vector<string> > extended_line_specification = make_map<gd_entry_type, vector<string> >
(INS,make_vector<string> (SEQ_ID)(POSITION)(INSERT_POSITION)(NEW_SEQ))
(MOB,make_vector<string> (SEQ_ID)(POSITION)(REPEAT_NAME)(STRAND)(DUPLICATION_SIZE)(INS_START)(INS_END)(DEL_START)(DEL_END)(MOB_REGION))
(AMP,make_vector<string> (SEQ_ID)(POSITION)(SIZE)(NEW_COPY_NUMBER)(MEDIATED)(MEDIATED_STRAND)(MOB_REGION))
(RA,make_vector<string>  (SEQ_ID)(POSITION)(INSERT_POSITION)(REF_BASE)(NEW_BASE))
(JC,make_vector<string>  (SIDE_1_SEQ_ID)(SIDE_1_POSITION)(SIDE_1_STRAND)(SIDE_2_SEQ_ID)(SIDE_2_POSITION)(SIDE_2_STRAND)(OVERLAP)(UNIQUE_READ_SEQUENCE))
;
  
enum diff_entry_field_variable_t {
  kDiffEntryFieldVariableType_BaseSequence,
  kDiffEntryFieldVariableType_PositiveInteger,
  kDiffEntryFieldVariableType_Integer,
  kDiffEntryFieldVariableType_Strand, // must be -1 or +1
  kDiffEntryFieldVariableType_PositiveInteger_ReverseSort,
};
  
map<string, diff_entry_field_variable_t > diff_entry_field_variable_types = make_map<string, diff_entry_field_variable_t>
(POSITION, kDiffEntryFieldVariableType_PositiveInteger)
(START, kDiffEntryFieldVariableType_PositiveInteger)
(END, kDiffEntryFieldVariableType_PositiveInteger)
(SIZE, kDiffEntryFieldVariableType_PositiveInteger_ReverseSort)
(STRAND, kDiffEntryFieldVariableType_Strand)
(NEW_SEQ, kDiffEntryFieldVariableType_BaseSequence)
(NEW_COPY_NUMBER, kDiffEntryFieldVariableType_PositiveInteger)
(DUPLICATION_SIZE, kDiffEntryFieldVariableType_Integer)
(DEL_START, kDiffEntryFieldVariableType_PositiveInteger)
(DEL_END, kDiffEntryFieldVariableType_PositiveInteger)
(INS_START, kDiffEntryFieldVariableType_BaseSequence)
(INS_END, kDiffEntryFieldVariableType_BaseSequence)
(INSERT_POSITION, kDiffEntryFieldVariableType_Integer)
(MEDIATED_STRAND, kDiffEntryFieldVariableType_Strand)
//JC item
(SIDE_1_POSITION, kDiffEntryFieldVariableType_PositiveInteger)
(SIDE_1_STRAND, kDiffEntryFieldVariableType_Strand)
(SIDE_2_POSITION, kDiffEntryFieldVariableType_PositiveInteger)
(SIDE_2_STRAND, kDiffEntryFieldVariableType_Strand)
(OVERLAP, kDiffEntryFieldVariableType_Integer)
(UNIQUE_READ_SEQUENCE, kDiffEntryFieldVariableType_BaseSequence)
;

const vector<string>gd_entry_type_lookup_table =
  make_vector<string>("UNKNOWN")("SNP")("SUB")("DEL")("INS")("MOB")("AMP")("INV")("CON")("RA")("MC")("JC")("CN")("UN")("CURA")("FPOS")("PHYL")("TSEQ")("PFLP")("RFLP")("PFGE")("NOTE")("MASK");

// Used when determining what fields need to be updated if ids are renumbered
// accounts for key=mutation_id:copy_index notation.
const vector<string> gd_keys_with_ids = 
  make_vector<string>("before")("within");
  
uint32_t cGenomeDiff::s_input_order_counter(0);  

////
// Begin sorting variables
////
// All fields must be assigned in this table and be required fields of the gd entries.
map<gd_entry_type, cGenomeDiff::sort_fields_item> diff_entry_sort_fields = make_map<gd_entry_type, cGenomeDiff::sort_fields_item>
(SNP,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(SUB,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(DEL,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(INS,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(MOB,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(AMP,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(INV,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(CON,  cGenomeDiff::sort_fields_item(1, SEQ_ID, POSITION))
(NOTE, cGenomeDiff::sort_fields_item(2, "note", "note"))
(RA,   cGenomeDiff::sort_fields_item(3, SEQ_ID, POSITION))
(MC,   cGenomeDiff::sort_fields_item(4, SEQ_ID, START))
(JC,   cGenomeDiff::sort_fields_item(5, SIDE_1_SEQ_ID, SIDE_1_POSITION))
(CN,   cGenomeDiff::sort_fields_item(6, SEQ_ID, START))
(UN,   cGenomeDiff::sort_fields_item(7, SEQ_ID, START))
(CURA, cGenomeDiff::sort_fields_item(8, "expert", "expert"))
(FPOS, cGenomeDiff::sort_fields_item(8, "expert", "expert"))
(PHYL, cGenomeDiff::sort_fields_item(8, "gd", "gd"))
(TSEQ, cGenomeDiff::sort_fields_item(8, "seq_id", "primer_1_start"))
(PFLP, cGenomeDiff::sort_fields_item(8, "seq_id", "primer_1_start"))
(RFLP, cGenomeDiff::sort_fields_item(8, "seq_id", "primer_1_start"))
(PFGE, cGenomeDiff::sort_fields_item(8, "seq_id", "enzyme"))
(MASK, cGenomeDiff::sort_fields_item(8, SEQ_ID, POSITION))
;

map<gd_entry_type, uint8_t> sort_order = make_map<gd_entry_type, uint8_t>
(SNP, 2)
(SUB, 4)
(DEL, 1)
(INS, 3)
(MOB, 5)
(AMP, 6)
(INV, 7)
(CON, 8)
(RA,  9)
(MC,  10)
(JC,  11)
(CN,  12)
(UN,  13)
(CURA, 14)
(FPOS, 15)
(PHYL, 16)
(TSEQ, 17)
(PFLP, 18)
(RFLP, 19)
(PFGE, 20)
(NOTE, 20)
(MASK, 20)
;
////
// End sorting variables
////
  
/*! Constructor.
 */
  
cDiffEntry::cDiffEntry()
: _type(UNKNOWN)
, _id("")
, _evidence()
{
}  
  
cDiffEntry::cDiffEntry(const gd_entry_type type)
: _type(type)
, _id("")
, _evidence()
{
}

cDiffEntry::cDiffEntry(const string &line, uint32_t line_number, cFileParseErrors* file_parse_errors)
  : _type(UNKNOWN)
  ,_id("")
  ,_evidence()
{
  cDiffEntry& de = *this;
  // this is a hidden field
  de["_line_number"] = to_string<uint32_t>(line_number);
  vector<string> tokens = split(line, "\t");

  if (tokens.size() < k_num_line_specification_common_prefix_columns) {
    if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Could not determine type, id, or parent_id.", true);
    return;
  }

  uint32_t COLUMN = 0;
  //Type.
  string type = tokens[COLUMN];
  RemoveLeadingTrailingWhitespace(type);
  de._type = cDiffEntry::type_to_enum(type);

  if (de._type == UNKNOWN) {
    if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Unknown type for entry.", true);
    return;
  }
  ++COLUMN;

  //Id.
  de._id = tokens[COLUMN];
  RemoveLeadingTrailingWhitespace(de._id);
  ++COLUMN;

  //Evidence.
  de._evidence = split(tokens[COLUMN], ",");
  for (uint32_t i = 0; i < de._evidence.size(); ++i) {
    RemoveLeadingTrailingWhitespace(de._evidence[i]);
  }
  ++COLUMN;

  //Specs.
  const vector<string>& specs = line_specification.find(de._type)->second;

  if (tokens.size() - COLUMN < specs.size() ) {
    if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Expected " + to_string(specs.size() + COLUMN) + " tab-delimited columns for entry", true);
    return;
  }
  
  for (uint32_t i = 0; i < specs.size(); ++i) {
      de[specs[i]] = tokens[COLUMN];
      RemoveLeadingTrailingWhitespace(de[specs[i]]);
      ++COLUMN;
  }

  //Fields.
  while(COLUMN < tokens.size() && tokens[tokens.size() - 1].size()) {
    cKeyValuePair kvp(tokens[COLUMN], '=');
    if (kvp.valid()) {
      string key   = kvp.get_key();
      string value = kvp.get_value();
      RemoveLeadingTrailingWhitespace(key);
      RemoveLeadingTrailingWhitespace(value);
      de[key] = value;
      
      
      // Certain keys are only allowed for specific entries
      if (key == MEDIATED) {
        if ( (de._type != DEL) && (de._type != AMP) && (de._type != SUB) ) {
          if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'mediated' is only allowed for entries of type DEL, AMP, or SUB.", true);
        }
      }
      
      if (key == MEDIATED_STRAND) {
        if ( (de._type != AMP) ) {
          if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'mediated_strand' is only allowed for entries of type AMP.", true);
        }
      }
      
      if (key == BETWEEN) {
        if ( (de._type != DEL) && (de._type != AMP) && (de._type != CON) ) {
          if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'between' is only allowed for entries of type MOB, AMP, or CON.", true);
        }
      }
      
      // Deprecated keys
        
      if (key == "nested_within") {
        if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'nested_within' is DEPRECATED and will be ignored. Use 'within' instead.", false);
      } else if (key == "nested_copy") {
        if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'nested_copy' IS DEPRECATED and will be ignored. Use 'within' instead.", false);
      } else if (key == "after") {
        if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'after' is DEPRECATED and will be ignored. Use 'within' or 'before' instead.", false);
      }
      
    } else {
      if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Field " + kvp + " is not a key=value pair. Ignoring this key.", false);
    }
    ++COLUMN;
  }
  
  // Certain keys must occur together (if one is there, the other had better be there)
  if (de._type == AMP) {
    if (de.entry_exists(MEDIATED) != de.entry_exists(MEDIATED_STRAND)) {
      if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Only one key of 'mediated' and 'mediated_strand' is supplied for this AMP. Both must be present to describe the mutation. Did you mean to use 'between' instead?", true);
    }
  }
  
  // Certain keys are only valid with certain entries
  if ( de.entry_exists(APPLY_SIZE_ADJUST) ) {
    if ( (de._type != AMP) && (de._type != DEL) && (de._type != SUB) && (de._type != CON) && (de._type != INV) ) {
      if (file_parse_errors) file_parse_errors->add_line_error(line_number, line, "Key 'apply_size_adjust' is only allowed for AMP, CON, DEL, INV, and SUB mutations.", true);
    }
  }
  
  return;
}
  
bool cDiffEntry::valid_id(string& test_id)  {

  // Record that we used this id, if it is currently valid
  int32_t test_id_int;
  bool is_int = is_integer(test_id, test_id_int);
  return (is_int && (test_id_int >= 1));
}
  
gd_entry_type cDiffEntry::type_to_enum(string type) {
  for (size_t i = 0; i < gd_entry_type_lookup_table.size(); i++) {
    if (type == gd_entry_type_lookup_table[i]) {
      return (gd_entry_type) i;
    }
  }
  return UNKNOWN;
}

// Checks all specification fields for expected types and adds errors to parse_errors
void cDiffEntry::valid_field_variable_types(cFileParseErrors& parse_errors) {
  
  const vector<diff_entry_key_t>& spec = extended_line_specification.count(this->_type)
  ? extended_line_specification.find(this->_type)->second : line_specification.find(this->_type)->second;
  
  uint32_t field_count = k_num_line_specification_common_prefix_columns; // skipping fixed fields
  for(map<string,string>::iterator it=this->begin(); it!=this->end(); ++it) {
    field_count++;
    string key = it->first;
    if (diff_entry_field_variable_types.count(key) == 0) continue;
    
    diff_entry_field_variable_t variable_type = diff_entry_field_variable_types[key];
    string value = it->second;
    
    if (variable_type == kDiffEntryFieldVariableType_BaseSequence) {
      if (!is_base_sequence(value))
        parse_errors.add_line_error(from_string<uint32_t>((*this)["_line_number"]), this->as_string(), "Expected base sequence for field containing only characters 'ATCGN' for field #" + to_string<uint32_t>(field_count) + " (" + key +  "): [" + key + "] instead of [" + value + "]." , true);
      continue;
    }
    
    /// Other types ---> are all numerical
    int32_t ret_val;
    bool integral = is_integer(value, ret_val);
    
    if (!integral) {
      parse_errors.add_line_error(from_string<uint32_t>((*this)["_line_number"]), this->as_string(), "Expected integral value for field " + to_string<uint32_t>(field_count) + ": [" + key + "] instead of [" + value + "]." , true);
      continue;
    }
    
    
    switch(variable_type)
    {
      case kDiffEntryFieldVariableType_PositiveInteger:
      case kDiffEntryFieldVariableType_PositiveInteger_ReverseSort:

        if (ret_val <= 0) { 
          parse_errors.add_line_error(from_string<uint32_t>((*this)["_line_number"]), this->as_string(), "Expected positive integral value for field " + to_string<uint32_t>(field_count) + ": [" + key + "] instead of [" + value + "]."  , true);
        }
        break;
        

        
      case kDiffEntryFieldVariableType_Strand:
        if ((ret_val != -1) && (ret_val != 1)) { 
          parse_errors.add_line_error(from_string<uint32_t>((*this)["_line_number"]), this->as_string(), "Expected strand value (-1/1) for field " + to_string<uint32_t>(field_count) + ": [" + key + "] instead of [" + value + "]." , true);
        }
        break;
        
      // already tested
      case kDiffEntryFieldVariableType_BaseSequence:
      case kDiffEntryFieldVariableType_Integer:
        break;  
    }
  }
}

//Static comparison function used for comparison operators
int32_t cDiffEntry::compare(const cDiffEntry& a, const cDiffEntry& b)
{
  gd_entry_type a_type = a._type;
  gd_entry_type b_type = b._type;
  
  //////////////////////////////////////////////////////////////////
  // First we sort according to output order
  
  cGenomeDiff::sort_fields_item a_sort_fields = diff_entry_sort_fields[a_type];
  cGenomeDiff::sort_fields_item b_sort_fields = diff_entry_sort_fields[b_type];
  
  if (a_sort_fields._f1 < b_sort_fields._f1) {
    return -1;
  } else if (a_sort_fields._f1 > b_sort_fields._f1) {
    return +1;
  }
  
  string a_sort_field_2 = a.find(a_sort_fields._f2)->second;
  string b_sort_field_2 = b.find(b_sort_fields._f2)->second;
  
  if (a_sort_field_2 < b_sort_field_2) {
    return -1;
  } else if (a_sort_field_2 > b_sort_field_2) {
    return +1;
  }
  
  uint32_t a_sort_field_3 = from_string<uint32_t>(a.find(a_sort_fields._f3)->second);
  uint32_t b_sort_field_3 = from_string<uint32_t>(b.find(b_sort_fields._f3)->second);
  
  if (a_sort_field_3 < b_sort_field_3) {
    return -1;
  } else if (a_sort_field_3 > b_sort_field_3) {
    return +1;
  }
  
  // Prefer certain mutation types before others
  uint8_t a_sort_order = sort_order[a_type];
  uint8_t b_sort_order = sort_order[b_type];
  
  if (a_sort_order < b_sort_order) {
    return -1;
  } else if (a_sort_order > b_sort_order) {
    return +1;
  }
  
  //////////////////////////////////////////////////////////////////
  // Then we sort for all fields defined in the line specification
  
  // Get full line spec
  vector<diff_entry_key_t> specs = extended_line_specification.count(a_type)
  ? extended_line_specification.find(a_type)->second : line_specification.find(a_type)->second;
  
  // always add these for uniqueness testing
  specs.push_back("phylogeny_id");
  specs.push_back("unique");
  specs.push_back("population_id");
  
  for(vector<diff_entry_key_t>::const_iterator it = specs.begin(); it != specs.end(); it++) {
    const diff_entry_key_t& spec(*it);
    
    bool a_exists = a.entry_exists(spec);
    bool b_exists = b.entry_exists(spec);
    
    // look for other breaks
    if (!a_exists && !b_exists) continue;
    if (!b_exists) return +1;
    if (!a_exists) return -1;
    
    // Perform the proper type of comparison
    // Default is a string if not provided...
    
    if (!diff_entry_field_variable_types.count(spec) ||
        (diff_entry_field_variable_types[spec] == kDiffEntryFieldVariableType_BaseSequence) ) {
      
      const string& a_val = a.find(spec)->second;
      const string& b_val = b.find(spec)->second;

      if (a_val < b_val)
        return -1;
      else if (a_val > b_val)
        return +1;
      
    } else {
      
      switch(diff_entry_field_variable_types[spec]) {
        case kDiffEntryFieldVariableType_PositiveInteger:
        case kDiffEntryFieldVariableType_PositiveInteger_ReverseSort:
        {
          uint32_t a_val = from_string<uint32_t>(a.find(spec)->second);
          uint32_t b_val = from_string<uint32_t>(b.find(spec)->second);

          if (a_val < b_val)
            return -1;
          else if (a_val > b_val)
            return +1;
          break;
        }
          
        case kDiffEntryFieldVariableType_Integer:
        case kDiffEntryFieldVariableType_Strand:
        {
          int32_t a_val = from_string<int32_t>(a.find(spec)->second);
          int32_t b_val = from_string<int32_t>(b.find(spec)->second);
          
          if (a_val < b_val)
            return -1;
          else if (a_val > b_val)
            return +1;
          break;
        }
          // handled above
        case kDiffEntryFieldVariableType_BaseSequence:
          break;
      }
    }
  }

  //////////////////////////////////////////////////////////////////
  // Return zero if the entries are equal (according to major specs)
  return 0;
}

bool cDiffEntry::is_mutation() const
{
  return gd_entry_type_lookup_table[_type].size() == 3;
}

bool cDiffEntry::is_evidence() const
{
  return gd_entry_type_lookup_table[_type].size() == 2;
}

bool cDiffEntry::is_validation() const
{
  return gd_entry_type_lookup_table[_type].size() == 4;
}
 
  
cReferenceCoordinate cDiffEntry::get_reference_coordinate_start() const
{
  switch (this->_type) {
    case SNP:
    case SUB:
    case DEL:
    case INV:
    case AMP:
    case CON:
    case MASK:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)));
    case INS:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)), this->entry_exists(INSERT_POSITION) ? from_string<uint32_t>(this->at(INSERT_POSITION)) : 1);
    case MOB:
    {
      if (from_string<uint32_t>(this->at("duplication_size")) == 0) {
        return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)), 1);
      } else {
        return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)));
      }
    }
    case RA:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)), from_string<uint32_t>(this->at(INSERT_POSITION)));
    case MC:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(START)));
    case UN:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(START)));
    default:
      ERROR("cDiffEntry::get_reference_coordinate_start not implemented for type: " + gd_entry_type_lookup_table[this->_type]);
  }
  return 0;
}
  
cReferenceCoordinate cDiffEntry::get_reference_coordinate_end() const
{
  switch (this->_type) {
    case SNP:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)));
    case SUB:
    case DEL:
    case INV:
    case AMP:
    case CON:
    case MASK:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)) + from_string<uint32_t>(this->at(SIZE)) - 1);
    case INS:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)), this->entry_exists(INSERT_POSITION) ? from_string<uint32_t>(this->at(INSERT_POSITION)) : 1);
    case MOB:
    {
      if (from_string<int32_t>(this->at("duplication_size")) == 0) {
        return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)) + from_string<int32_t>(this->at("duplication_size")), 1);
      } else {
        return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)) + abs(from_string<int32_t>(this->at("duplication_size"))) - 1);
      }
    }
    case RA:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(POSITION)), from_string<uint32_t>(this->at(INSERT_POSITION)));
    case MC:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(END)));
    case UN:
      return cReferenceCoordinate(from_string<uint32_t>(this->at(END)));
    
    default:
      ERROR("cDiffEntry::get_reference_coordinate_end not implemented for type: " + gd_entry_type_lookup_table[this->_type]);
  }
  return 0;
}
  
bool cDiffEntry::located_within(const cDiffEntry &within) const
{
  return(
         (this->get(SEQ_ID) == within.get(SEQ_ID))
         && (this->get_reference_coordinate_start() >= within.get_reference_coordinate_start())
         && (this->get_reference_coordinate_end() <= within.get_reference_coordinate_end())
         );
}
  
int32_t cDiffEntry::mutation_size_change(cReferenceSequences& ref_seq_info) const
{
  int32_t size_change(0);
  
  switch (this->_type) {
    
    case SNP:
      size_change = 0;
      break;
      
    case SUB:
      size_change = - from_string<uint32_t>(this->get(SIZE));
      if (this->entry_exists(APPLY_SIZE_ADJUST)) {
        size_change -= from_string<int32_t>(this->get(APPLY_SIZE_ADJUST));
        ASSERT(size_change < 0, "Mutation has zero or negative size after adding APPLY_SIZE_ADJUST:\n" + this->as_string() );
      }
      size_change += this->get(NEW_SEQ).length();
      break;
      
    case INS:
      size_change = this->get(NEW_SEQ).length();
      break;
      
    case DEL:
      size_change = -(from_string<uint32_t>(this->get(SIZE)));
      if (this->entry_exists(APPLY_SIZE_ADJUST)) {
        size_change -= from_string<int32_t>(this->get(APPLY_SIZE_ADJUST));
        ASSERT(size_change < 0, "Mutation has zero or negative size after adding APPLY_SIZE_ADJUST:\n" + this->as_string() );
      }
      break;
      
    case INV:
      size_change = 0;
      break;
      
    case MASK:
      size_change = 0;
      break;
      
    case AMP:
    {
      size_change = from_string<uint32_t>(this->get(SIZE)) * (from_string<uint32_t>(this->get("new_copy_number")) - 1);
      
      // Special case of mediated AMP
      if (this->entry_exists(MEDIATED)) {
        
        cFeatureLocation repeat_feature_picked;
        string seq_id_picked;
        string mediated_string;
        string mob_region;
        
        if (this->entry_exists(MOB_REGION)) {
          mob_region = this->get(MOB_REGION);
        }
        
        mediated_string = ref_seq_info.repeat_family_sequence(this->get(MEDIATED), from_string<int16_t>(this->get(MEDIATED_STRAND)), mob_region.size() ? &mob_region : NULL, &seq_id_picked, &repeat_feature_picked);
        
        size_change += mediated_string.size() * (from_string<uint32_t>(this->get("new_copy_number")) - 1);
      }
      
      if (this->entry_exists(APPLY_SIZE_ADJUST)) {
        size_change += from_string<int32_t>(this->get(APPLY_SIZE_ADJUST)) * (from_string<uint32_t>(this->get("new_copy_number")) - 1);
        ASSERT(size_change > 0, "Mutation has zero or negative size after adding APPLY_SIZE_ADJUST:\n" + this->as_string() );
      }
      break;
    }

    case MOB:
    {
      // @JEB: Important: repeat_size is not a normal attribute and must be set before calling this function
      //       Also: this size includes the target site duplication
      ASSERT(this->entry_exists("repeat_size"), "Repeat size field does not exist for entry:\n" + this->as_string());
      size_change = from_string<int32_t>(this->get("repeat_size")) + from_string<int32_t>(this->get("duplication_size"));
      if (this->entry_exists("del_start"))
        size_change -= from_string<uint32_t>(this->get("del_start"));
      if (this->entry_exists("del_end"))
        size_change -= from_string<uint32_t>(this->get("del_end"));
      if (this->entry_exists("ins_start"))
        size_change += this->get("ins_start").length();
      if (this->entry_exists("ins_end"))
        size_change += this->get("ins_end").length();
      break;
    }
      
    case CON:
    {
      uint32_t replace_target_id, replace_start, replace_end;
      ref_seq_info.parse_region(this->get("region"), replace_target_id, replace_start, replace_end);
      size_change = from_string<uint32_t>(this->get(SIZE));
      
      if (this->entry_exists(APPLY_SIZE_ADJUST)) {
        size_change += from_string<int32_t>(this->get(APPLY_SIZE_ADJUST));
        ASSERT(size_change > 0, "Mutation has zero or negative size after adding APPLY_SIZE_ADJUST:\n" + this->as_string() );
      }
      
      int32_t new_size = (replace_end > replace_start) ? replace_end - replace_start + 1 : replace_start - replace_end + 1;
      size_change =  new_size - size_change;
      break;
    }
      
    default:
      ASSERT(false, "Unable to calculate mutation size change for this type of entry:\n" + this->as_string());
      return UNDEFINED_INT32;
  }

  return size_change;
}

// shift_offset normally means the position of the mutation doing the shifting
// shift_offset of -1 means we are within the current mutation 
//   => we don't change its size, but we may shift its position in a special way for AMP/MOB/INS
//   inset_pos is the index after the current position // = 0 for actually at this position
//
// shift_size is the change in size within the specified interval
//
// shift_replace size is the size of the new sequence being inserted
//
//   For an INS mutation =>
//      shift_offset = position
//      insert_pos = 1 (meaning after the base referred to in shift_offset)
//      shift_size = number of bases inserted
//

void cDiffEntry::mutation_shift_position(const string& seq_id, const cReferenceCoordinate& shift_start, const cReferenceCoordinate& shift_end, int32_t shift_size)
{
  
  ASSERT(this->is_mutation(), "Attempt to shift position of non-mutation cDiffEntry");

  // negative shift_size means deletion; positive shift_size means insertion
  if (shift_size == 0) return;
  if (seq_id != (*this)[SEQ_ID]) return;
  
  // anything that has a 'size' potentially needs to be adjusted if the shift position and size overlaps        
  cReferenceCoordinate original_start = this->get_reference_coordinate_start();
  cReferenceCoordinate original_end = this->get_reference_coordinate_end();
  cReferenceCoordinate final_start = original_start;
  cReferenceCoordinate final_size = original_end - original_start + cReferenceCoordinate(1);
        
  // Special case where the mutation is 'within' this mutation
  if (shift_start.get_position() < 0) {

    if ((shift_start <= original_start) && (shift_end >= original_end)) {
      // change overlaps both sides
      // ERROR("Deletion completely encompasses other mutation");
      final_start = shift_start;
      final_size = 0;
    } else if ((original_start >= shift_start) && (original_start <= shift_end)) {
      // change overlaps left side
      final_start = shift_start;
      final_size = original_end - original_start + 1 - (shift_end - original_start);
    } else if ((original_end >= shift_start) && (original_end <= shift_end)) {
      // change overlaps right side
      final_start = original_start;
      final_size = shift_start - original_start;
    } else if ((shift_start > original_start) && (shift_end < original_end)) {
      // change is contained within 
      final_start = original_start;
      final_size = original_end - original_start + 1 + shift_size; // original size minus size of deletion
    } else if (original_start >= shift_start) {
      final_start = original_start + shift_size;
    }
    
  // Normal case where we shift the mutation
  } else {
  
    if ((shift_start >= original_start) && (shift_end <= original_end)) {
      final_size = original_end - original_start + 1 + shift_size;
    } else if (original_start >= shift_start) {
      final_start = original_start + shift_size;
    }
  }

  (*this)[POSITION] = to_string(final_start.get_position());
  if (this->entry_exists(SIZE)) {
    (*this)[SIZE] = to_string(final_size.get_position());
  }
}

  
// Reverse-complements without changing position
void cDiffEntry::mutation_reverse_complement() {
  
  switch (this->_type) {
      
    // Nothing special for these cases
    case DEL:
    case INV:
    case AMP:
      break;

    case SNP:
    case SUB:
    case INS:
      (*this)[NEW_SEQ] = reverse_complement((*this)[NEW_SEQ]);
      break;
      
    case CON:      
    {
      // flip coordinates of region
      string seq_id;
      uint32_t start_pos_1, end_pos_1;
      cReferenceSequences::parse_region((*this)[REGION], seq_id, start_pos_1, end_pos_1);
      (*this)[REGION] = seq_id + ":" + s(end_pos_1) + "-" + s(start_pos_1);      
      break;
    } 
    case MOB:
    {
      (*this)[STRAND] = s(-n((*this)[STRAND]));
      string del_start = this->entry_exists(DEL_START) ? (*this)[DEL_START] : "";
      string del_end = this->entry_exists(DEL_END) ? (*this)[DEL_END] : "";
      string ins_start = this->entry_exists(INS_START) ? reverse_complement((*this)[INS_START]) : "";
      string ins_end = this->entry_exists(INS_END) ? reverse_complement((*this)[INS_END]) : "";
      this->erase(DEL_START);
      this->erase(DEL_END);
      this->erase(INS_START);
      this->erase(INS_END);
      
      // flips start and end
      if (!del_end.empty()) (*this)[DEL_START] = del_end;
      if (!del_start.empty()) (*this)[DEL_END] = del_start;
      if (!ins_end.empty()) (*this)[INS_START] = ins_end;
      if (!ins_start.empty()) (*this)[INS_END] = ins_start;
      break;
    }
      
    default:
      ERROR("Attempt to reverse complement cDiffEntry of unsupported type:" + to_string(this->_type));

  }
}
  
// Updates positions and reverse-complements
void cDiffEntry::mutation_invert_position_sequence(cDiffEntry& inverting_mutation) {
  ASSERT(this->is_mutation(), "Attempt to invert position of non-mutation cDiffEntry");
  
  // are we on the right sequence fragment?
  if (inverting_mutation[SEQ_ID] != (*this)[SEQ_ID]) return;
  
  int32_t start_inversion = from_string<int32_t>(inverting_mutation[POSITION]);
  int32_t end_inversion = start_inversion + from_string<int32_t>(inverting_mutation[SIZE]) - 1;

  int32_t position = from_string<int32_t>((*this)[POSITION]);
  
  // Flip things that are totally contained
  // Error if something overlaps the edges
  // shift_replace_size has the size of the inversion (shift_size = 0)
  
  // Size should be how many reference bases the mutation affects!
  // It will be one for a SNP, etc., insertion, but we won't use it.
  int32_t ref_size = this->get_reference_coordinate_end().get_position() - this->get_reference_coordinate_start().get_position() + 1;
  
  // Does not overlap
  if (position + ref_size < start_inversion)
    return;
  else if (position > end_inversion)
    return;
  
  // Contained within, invert coordinates
  else if ((position >= start_inversion) && (position + ref_size - 1 <= end_inversion)) {
    
    //Reverse complement the mutation
    this->mutation_reverse_complement();
    
    if (this->_type==INS) {
    
      int32_t insert_pos;
      if (this->entry_exists(INSERT_POSITION))
        insert_pos = from_string<int32_t>((*this)[INSERT_POSITION]);
    
    // case of INS, because it refers to between positions, we need to substract an extra one from the position
    // We also need to flip insert_pos order for any mutations that are strung together with multiple insert counts
    // (we do this by taking the negative). We must re-sort after applying an INV for this reason!!
      
      (*this)[POSITION] = to_string<int32_t>(end_inversion - (position - start_inversion) - 1);
      if (this->entry_exists(INSERT_POSITION))
        (*this)[INSERT_POSITION] = to_string<int32_t>(-insert_pos);
      
    } else { // other cases
      
      (*this)[POSITION] = to_string<int32_t>(end_inversion - (position + (ref_size - 1) - start_inversion));
      
    }
    return;
  }
  
  // Overlaps end
  else {
    WARN("This mutation:\n" + this->as_string() + "\nextends across an endpoint of inversion:\n" + inverting_mutation.as_string() + "\nWhen applying the inversion, its sequence will not be reverse complemented, and its coordinates will not be shifted.");
  }
  
}
  
  
/* Used for marking 'adjacent' mutations in APPLY/NORMALIZE and could be used for COUNT if merged into that code
*/
  
bool cDiffEntry::is_small_mutation(uint32_t large_size_cutoff)
{
  cDiffEntry& mut = *this;

  if (mut._type == SNP) {
    return true;
  }
  
  if (mut._type == DEL) {
    if (from_string<uint32_t>(mut[SIZE]) <= large_size_cutoff) {
      return true;
    }
    
    if (mut.entry_exists(REPEAT_NEW_COPIES)) {
      return true;
    }
  }
  
  if (mut._type == INS) {

    if (mut[NEW_SEQ].size() <= large_size_cutoff) {
      return true;
    }
    
    if (mut.entry_exists(REPEAT_NEW_COPIES)) {
      return true;
    }
  }
  
  if (mut._type == SUB) {
    int32_t old_size = from_string<int32_t>(mut[SIZE]);
    int32_t new_size = mut[NEW_SEQ].size();
    
    if (static_cast<uint32_t>(abs(new_size - old_size)) <= large_size_cutoff) {
      return true;
    }
  }
    
  return false;
}
  
/*! Add 'mediate', 'adjacent', and between tags to a mutation
*/
  
void cDiffEntry::annotate_repeat_hotspots(cReferenceSequences& new_ref_seq_info, int32_t slop_distance, int32_t size_cutoff_AMP_becomes_INS_DEL_mutation, bool remove_old_tags, bool warn_after_mode)
{
  cDiffEntry& mut = *this;
  
  if (remove_old_tags) {
    mut.erase("adjacent");
    mut.erase("between");
    mut.erase("mediated");
  }
  
  
  map<string,string> nearby_tags;
  if (mut.entry_exists("adjacent")) nearby_tags["adjacent"] = mut["adjacent"];
  if (mut.entry_exists("between")) nearby_tags["between"] = mut["between"];
  if (mut.entry_exists("mediated")) nearby_tags["mediated"] = mut["mediated"];
  
  cAnnotatedSequence& this_seq = new_ref_seq_info[mut[SEQ_ID]];
  
  int64_t mut_start_1 = mut.get_reference_coordinate_start().get_position();
  int64_t mut_end_1 = mut.get_reference_coordinate_end().get_position();
  
  string both_close_key = "ignore";
  string one_close_key = "ignore";
  
  // Only these types can be 'within' and 'between' and have a SIZE attribute
  if ((mut._type == DEL) || (mut._type == AMP)) {
    int64_t size = from_string<int64_t>(mut[SIZE]);
    
    // short ones aren't mediated, just adjacent
    if (size > size_cutoff_AMP_becomes_INS_DEL_mutation) {
      both_close_key = "between";
      one_close_key = "mediated";
      
      if (mut._type == AMP) {
        one_close_key = "ignore"; // this ignores it
      }
    }
  }
    
  if (this->is_small_mutation()) {
    both_close_key = "adjacent";
    one_close_key = "adjacent";
  }
  
  if (mut._type == MOB) {
    one_close_key = "ignore"; // this ignores it
    both_close_key = "ignore";
  }
  
  // We make no assumptions about the directions of relevant IS elements in between/mediated here.
  int32_t tmp_slop_distance = slop_distance;
  cFeatureLocation* start_repeat = cReferenceSequences::find_closest_repeat_region_boundary(mut_start_1, this_seq.m_repeats, tmp_slop_distance, -1, true);
  if (start_repeat == NULL) {
    tmp_slop_distance = slop_distance;
    start_repeat = cReferenceSequences::find_closest_repeat_region_boundary(mut_start_1, this_seq.m_repeats, tmp_slop_distance, 1, true);
  }
  
  tmp_slop_distance = slop_distance;
  cFeatureLocation* end_repeat = cReferenceSequences::find_closest_repeat_region_boundary(mut_end_1, this_seq.m_repeats, tmp_slop_distance, 1, true);
  if (end_repeat == NULL) {
    tmp_slop_distance = slop_distance;
    end_repeat = cReferenceSequences::find_closest_repeat_region_boundary(mut_end_1, this_seq.m_repeats, tmp_slop_distance, -1, true);
  }
  
  if ((start_repeat != NULL) && (end_repeat != NULL)) {
    if ( (*(start_repeat->get_feature()))["name"] != (*(end_repeat->get_feature()))["name"]) {
      // different names is an odd case - WARN and don't assign anything
      WARN("Mutation has boundaries near two different repeat families, saving only the first one:\n" + mut.as_string());
      nearby_tags[one_close_key] = (*(start_repeat->get_feature()))["name"];
    } else {
      nearby_tags[both_close_key] = (*(start_repeat->get_feature()))["name"];
    }
    
  } else if (start_repeat != NULL) {
    nearby_tags[one_close_key] = (*(start_repeat->get_feature()))["name"];
  } else if (end_repeat != NULL) {
    nearby_tags[one_close_key] = (*(end_repeat->get_feature()))["name"];
  }
  
  if (!warn_after_mode) {
    // Finally reassign (this assumes none are removed)
    if (nearby_tags.count("adjacent")) mut["adjacent"] = nearby_tags["adjacent"];
    if (nearby_tags.count("between")) mut["between"] = nearby_tags["between"];
    if (nearby_tags.count("mediated")) mut["mediated"] = nearby_tags["mediated"];
  } else {
    // In this mode we just show a warning
    
    // Be careful to not set these accidentally to NULL by looking them up
    // when they don't exist
    bool was_adjacent = mut.entry_exists("adjacent");
    bool was_between = mut.entry_exists("between");
    bool was_mediated = mut.entry_exists("mediated");

    if (mut["adjacent"] != nearby_tags["adjacent"]) {
      WARN("Possible 'adjacent' tag should be added with value (" + nearby_tags["adjacent"] + ") due to later mutation:\n" + mut.as_string());
    }
    
    // Why these checks? Deletion that is between two IS looks like mediated after it occurs...
    if (!mut.entry_exists("between") && !mut.entry_exists("mediated")) {
    
      if ((mut["between"] != nearby_tags["between"])) {
        WARN("Possible 'between' tag should be added due to later mutation:\n" + mut.as_string());
      }
      if (mut["mediated"] != nearby_tags["mediated"]) {
        WARN("Possible 'mediated' tag should be added due to later mutation:\n" + mut.as_string());
      }

    }
    
    if (!was_adjacent) mut.erase("adjacent");
    if (!was_between) mut.erase("between");
    if (!was_mediated) mut.erase("mediated");
  }
}
  
  
/*! Marshal this diff entry into an ordered list of fields.
 */
void cDiffEntry::marshal(vector<string>& s) const {
  s.push_back(gd_entry_type_lookup_table[_type]);
  s.push_back(_id);
  
  // Use a dot "." if no evidence is provided
  string evidence_string = join(_evidence, ",");
  s.push_back(evidence_string.size() ? evidence_string : ".");

  // deep copy all fields:
  cDiffEntry cp= *this;
  
  // marshal specified fields in-order, removing them from the copy after they've 
  // been printed:
  
  const vector<string>& f = line_specification.find(_type)->second;
  
  for (vector<string>::const_iterator it=f.begin(); it != f.end(); it++)
  {
    diff_entry_map_t::iterator iter=cp.find(*it);
    
    ASSERT(iter != cp.end(), "Did not find required field '" + *it + "' to write in entry id " + _id + " of type '" + gd_entry_type_lookup_table[_type] + "'.");
    
    s.push_back(iter->second);
    cp.erase(iter);
    
  }
  
  // marshal whatever's left, unless it _begins with an underscore or is empty (a placeholder)
  for(diff_entry_map_t::iterator i=cp.begin(); i!=cp.end(); ++i) {
    
    assert(i->first.size());
    if (i->first.substr(0,1) == "_") continue;
    if (i->second.empty()) continue;
    
    // Be sure the entry is non-empty! Would rather have this as a check.
    /*
    ASSERT(i->second.size(), "Attempt to write genome diff entry with blank value in key=value pair where [key]=" + i->first + "\n" + join(s,"\t"));
    */
    
    s.push_back(i->first + "=" + i->second);
  }
}

// Created the line to be printed
string cDiffEntry::as_string(void) const
{
  vector<string> fields;
  marshal(fields);
  return join(fields, "\t");
}
 
size_t cDiffEntry::number_reject_reasons()
{
  if(this->entry_exists(REJECT))
  {
    return this->get_reject_reasons().size();
  }
  return 0;
}

vector<string> cDiffEntry::get_reject_reasons(const string field)
{
  vector<string> return_value;
  if (this->entry_exists(field)) {
    return_value = split((*this)[field], ",");
  } 
  return return_value;
}

/*! Add reject reason to diff entry.
 */
void cDiffEntry::add_reject_reason(const string &reason) {

  if (!this->entry_exists(REJECT)) {
    (*this)[REJECT] = reason;
  }
  // exists already, make comma separated list
  else {
    vector<string> current_reject_reasons = split((*this)[REJECT], ",");
    bool reject_exists = false;
    for (vector<string>::iterator it=current_reject_reasons.begin(); it != current_reject_reasons.end(); it++) {
      if (*it == reason) reject_exists = true;
    }
    if (!reject_exists) {
      current_reject_reasons.push_back(reason);
      (*this)[REJECT] = join(current_reject_reasons, ",");
    }
  }
}
  
/*! Add reject reason to diff entry.
 */
void cDiffEntry::remove_reject_reason(const string &reason) {
  
  if (!this->entry_exists(REJECT)) return;
  
  vector<string> current_reject_reasons = split((*this)[REJECT], ",");
  vector<string> new_reject_reasons;
  
  for (vector<string>::iterator it=current_reject_reasons.begin(); it != current_reject_reasons.end(); it++) {
    if (*it != reason) {
      new_reject_reasons.push_back(*it);
    }
  }
  
  if (current_reject_reasons.size() > 0) {
    (*this)[REJECT] = join(current_reject_reasons, ",");
  }
}
  
  
//! Remove all information except required fields
cDiffEntry cDiffEntry::to_spec() const
{
  cDiffEntry de(_type);
  
  // Get full line spec
  const vector<diff_entry_key_t>& specs = extended_line_specification.count(_type)
  ? extended_line_specification.find(_type)->second : line_specification.find(_type)->second;
  
  for(vector<diff_entry_key_t>::const_iterator it = specs.begin(); it != specs.end(); it++) {
    const diff_entry_key_t& spec(*it);
    
    if (this->count(spec)) {
      de[spec] = this->get(spec);
    }
  }
  
  return de;
}
  
void cDiffEntry::strip_to_spec()
{
  cDiffEntry de(_type);
  de = this->to_spec();
  *this = de;
}
  
void cDiffEntry::normalize_to_sequence(const cAnnotatedSequence &sequence, bool verbose)
{
  //! Step: Initialize parameters.
  //Only diff entries applicable to de sequence can be considered.
  assert(this->entry_exists("seq_id") || (*this)["seq_id"] == sequence.m_seq_id);
  assert(this->entry_exists("position"));
  
  //Sequences should be viewed as having index + one offset.
  const uint32_t pos_1 = strtoul((*this)["position"].c_str(), NULL, 0);
  if (!pos_1) {
    this->insert(pair<string, string>("comment_out", "True"));
    return;
  }
  
  /*! Step: Type specific normalizations. For some, the initial parameters given
   can be altered to be normalized, for others the parameters can't be altered
   and the mutation is not valid for the given reference sequence. */
  switch (this->_type)
  {
    case DEL:
    {
      /*     
       +++EXAMPLE DEL: pos_1 = 1, size = 3
       
       START:
       
       v pos_2
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       
       seq_1 = ACT
       seq_2 = ACT
       NOT VALID, ACT == ACT
       
       NEXT(pos_1++):
       
       v pos_2
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       
       seq_1 = CTA
       seq_2 = CTA
       NOT VALID, CTA == CTA
       
       NEXT(pos_1++):
       
       v pos_2
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       
       seq_1 = TAC
       seq_2 = TAT
       ^ first mismatch, normalize pos_1 to here.
       
       THEREFOR: pos_1 = 8. 
       */
      
      assert(this->entry_exists("size"));
      
      //! Step: Initialize parameters.
      typedef string::const_iterator base_itr_t;
      typedef pair<base_itr_t, base_itr_t> base_pair_t;
      const size_t n = strtoul((*this)["size"].c_str(), NULL, 0);
      
      if (!n) {
        this->insert(pair<string, string>("comment_out", "True"));
        return;
      }
      
      /*! Step: Attempt to normalize the start position by iterating through new
       start positions. */
      uint32_t norm_pos_1 = pos_1;
      for(int i = 0;;++i) {
        
        uint32_t seq1_pos_1 = pos_1 + i;
        uint32_t seq1_end_1 = seq1_pos_1 + n - 1;
        
        uint32_t seq2_pos_1 = seq1_end_1 + 1;
        uint32_t seq2_end_1 = seq2_pos_1 + n - 1;
        
        const string &seq1 =
        sequence.get_circular_sequence_1(seq1_pos_1, n);
        assert(seq1.size() == n);
        
        const string &seq2 =
        sequence.get_circular_sequence_1(seq2_pos_1, n);
        assert(seq2.size() == n);
        
        
        const base_pair_t &base_pair =
        mismatch(seq1.begin(), seq1.end(), seq2.begin());
        
        bool valid = base_pair.first != seq1.end();
        
        
        if (valid) {
          norm_pos_1 += i + (base_pair.first - seq1.begin());
          sprintf((*this)["position"], "%u", norm_pos_1);
          if (verbose) {
            sprintf((*this)["norm_pos"], "%u_to_%u", pos_1, norm_pos_1);
          }
        }
        
        if (verbose) {
          if (valid && norm_pos_1 == pos_1) {
            cerr << "VALID POSITION: " << pos_1 << endl;
          }
          else if (valid && norm_pos_1 != pos_1) {
            cerr << "NORMALIZED POSITON: " << pos_1 << " to " << norm_pos_1 << ' ' << endl;
          } else {
            cerr << "INVALID POSITION: " << pos_1 << endl;
          }
          
          cerr << "[Mutation]: " << this->to_spec().as_string() << endl;
          cerr << "Sequence 1 [" << seq1_pos_1 << '-' << seq1_end_1 << "]: " << seq1 << endl;
          
          cerr << "Sequence 2 [" << seq2_pos_1 << '-' << seq2_end_1 << "]: " << seq2 << endl;
          
          cerr << "Deleted Sequence [" << norm_pos_1 << '-' << norm_pos_1 + n - 1 << "]: "; 
          cerr << sequence.get_circular_sequence_1(norm_pos_1, n) << endl;
          
          string prev_seq = sequence.get_circular_sequence_1(norm_pos_1 - 10, 20);
          string left_seq = sequence.get_circular_sequence_1(norm_pos_1 - 10, 10);
          string right_seq = sequence.get_circular_sequence_1(norm_pos_1 + n , 10);
          
          cerr << "Previous Sequence:  " << prev_seq << endl;
          cerr << "Resulting Junction: " << left_seq << '\t' << right_seq << endl;
          
          cerr << endl;
        }
        
        if (valid) {
          break;
        }
        
      }
      
    } break;
      
    case INS:
    {
      /*
       +++EXAMPLE INS: pos_1 = 1, new_seq = CAA
       
       START:
       
       v pos_2
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       
       seq_1 = CAA
       seq_2 = CTA
       ^ match, could be predicted as a SNP, NOT VALID
       
       NEXT(pos_1++):
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       seq_1 = CAA
       seq_2 = TAC
       ^ mismatch, won't be predicted as a SNP, normalize pos_1 to here.
       
       THEREFOR: pos_1 = 2.
       
       
       +++EXAMPLE INS -> AMP: pos_1 = 1, new_seq = CTA
       
       START:
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       seq_1 = CTA
       seq_2 = CTA
       NOT VALID, CTA == CTA, possible AMP.
       
       NEXT(pos_1 += 3):
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       seq_1 = CTA
       seq_2 = CTA
       NOT VALID, CTA == CTA, possible AMP.
       
       NEXT(pos_1 += 3):
       Ref: ACTACTATCACACTAATACAATA
       ^ pos_1
       seq_1 = CTA
       seq_2 = TCA
       VALID, Passes SNP check and will be evaluated as an AMP.
       
       THEREFOR: INS->AMP, pos_1 = 1, size = 3, new_copy_number = 2, orig_type = INS.
       
       */
      assert(this->entry_exists("new_seq"));
      
      //! Step: Initialize parameters.
      const string first = (*this)["new_seq"];
      const size_t n = first.size();
      assert(n);
      
      /*! Step: Attempt to normalize the start position by iterating through new
       start positions. */
      uint32_t i = pos_1;
      
      bool bAmp, bSnp;
      bAmp = true;
      bSnp = true;
      
      while(bAmp || bSnp)
      {
        bAmp = true;
        bSnp = true;
        
        for(;;i += n)
        {
          const string second = sequence.get_circular_sequence_1(i + 1, n);
          assert(second.size());
          
          if (first != second)  {
            bAmp = false;
            break;  }
        }
        
        for(;;i += 1)
        {
          const string second = sequence.get_circular_sequence_1(i + 1, n);
          assert(second.size());
          
          if (first[0] != second[0])  {
            bSnp = false;
            break;  }
        }
      }
      
      //! Step: Determine if the start pos needs to be normilized.
      bool is_new_position = pos_1 != i;
      if (is_new_position) {
        sprintf((*this)["position"], "%u", i);
        sprintf((*this)["norm_pos"], "%u_to_%u", pos_1, i);
      }
      
      //! Step: Determine if this is actually an AMP.
      if ((sequence.get_circular_sequence_1(i - (n - 1), n) == first)  && (n > 1)) {
        this->_type = AMP;
        sprintf((*this)["size"], "%u", n);
        sprintf((*this)["new_copy_number"], "2");
        sprintf((*this)["orig_type"], "INS");
      }
    } break;
      
    case SNP:
    {
      assert(this->entry_exists("new_seq"));
      assert((*this)["new_seq"].size() == 1);
      
      //! Step: Initialize parameters.
      const base_char first = (*this)["new_seq"][0];
      
      //! Step: Compare bases.
      const base_char second = sequence.get_circular_sequence_1(pos_1, 1)[0];
      
      //! Step: If bases are not equal then it's not a valid snp.
      bool is_base_not_equal = (first != second);
      if (!is_base_not_equal) {
        sprintf((*this)["norm"], "is_not_valid");
      }
    } break;
      
    case SUB:
    {
      assert(this->entry_exists("size"));
      assert(this->entry_exists("new_seq"));
      
      //! Step: Initialize parameters.
      const size_t n = strtoul((*this)["size"].c_str(), NULL, 0);
      assert(n);
      const string first = (*this)["new_seq"];
      assert(first.size());
      
      const string second = sequence.get_circular_sequence_1(pos_1, n);
      const string third = sequence.get_circular_sequence_1(pos_1 + n, 1);
      
      if (first.find_first_of(second) != string::npos &&
          first.find_first_of(third) != string::npos ) {
        sprintf((*this)["norm"], "is_not_valid");
      }
    } break;
      
    case MOB:
    {
      assert(this->entry_exists("repeat_name"));
      assert(this->entry_exists("strand"));
      assert(this->entry_exists("duplication_size"));
    } break;
      
    case INV:
    {
      assert(this->entry_exists("size"));
      const size_t n =  strtoul((*this)["size"].c_str(), NULL, 0);
      assert(n);
    } break;
      
    case AMP:
    {
      assert(this->entry_exists("size"));
      assert(this->entry_exists("new_copy_number"));
    } break;
      
    case CON:
    {
      assert(this->entry_exists("size"));
      assert(this->entry_exists("region"));
    } break;
      
      
    default:
      break;
      
  }//End switch.
  
  return;
}


  

/*! Constructor.
 */
cGenomeDiff::cGenomeDiff(const string& filename)
  : _unique_id_counter(0) 
{
 read(filename);  
}

/*! Merge Constructor.
 */
cGenomeDiff::cGenomeDiff(cGenomeDiff& merge1, cGenomeDiff& merge2, bool unique, bool new_id, bool verbose)
 : _unique_id_counter(0)
{
  this->merge(merge1, unique, new_id, verbose);
  this->merge(merge2, unique, new_id, verbose);
}
  
diff_entry_ptr_t cGenomeDiff::parent(const cDiffEntry& evidence)
{
  for(diff_entry_list_t::iterator itr_test_item = _entry_list.begin();
      itr_test_item != _entry_list.end(); itr_test_item ++) { 
    
    cDiffEntry& test_item = **itr_test_item;
    
    for(vector<string>::iterator itr = test_item._evidence.begin();
        itr != test_item._evidence.end(); itr ++) { 
      string& test_evidence_id = (*itr);
      if(test_evidence_id == evidence._id) {      
        return diff_entry_ptr_t(*itr_test_item);
      }
    }
  }
  return diff_entry_ptr_t(NULL);
}
  
/*! Read a genome diff(.gd) from the given file to class member
 _entry_list
 */
cFileParseErrors cGenomeDiff::read(const string& filename, bool suppress_errors) {
  _file_path = filename;
  
  // title (old base_name) defaults to file name with no extension
  this->set_title(cString(filename).get_base_name_no_extension(true));
  
  ifstream in(get_file_path().c_str());

  ASSERT(in.good(), "Could not open file for reading: " + get_file_path());
  uint32_t line_number = 0;
  cFileParseErrors parse_errors(get_file_path());
  
  //! Step: Handle header parameters.
  //Example header:
  //#=GENOME_DIFF 1.0
  //#=AUTHOR    Barrick JE
  //#=REFSEQ    Genbank:NC_012967.1
  //#=READSEQ   SRA:SRR066595
  
  /*#=GENOME_DIFF version must be initialized for this file to be recognized
   as a genome diff file. */
  metadata.version = "";
  while(in.peek() == '#') {
    in.get();
    if (in.peek() != '=') {
      in.unget();
      break;
    } else {
      in.unget();
    }
    
    string whole_line = "";
    string second_half = "";
    
    getline(in, whole_line);
    line_number++;
    
    vector<string> split_line = split_on_whitespace(whole_line);
    
    if(split_line.size() > 1)second_half = split_line[1];
    for(size_t j = 2; j < split_line.size(); j++)  {
      second_half += " " + split_line[j];  }    
    
    //Evaluate key.
    if (split_line[0] == "#=GENOME_DIFF" && split_line.size() > 1) {
      metadata.version = second_half;
    }
    else if (split_line[0] == "#=AUTHOR") { 
      metadata.author = second_half;
    }
    else if (split_line[0] == "#=CREATED") {
      metadata.created = second_half;
    }
    else if (split_line[0] == "#=PROGRAM") {
      metadata.program = second_half;
    }
    else if (split_line[0] == "#=COMMAND") {
      metadata.command = second_half;
    }
    else if (split_line[0] == "#=REFSEQ" && split_line.size() > 1) {
      metadata.ref_seqs.push_back(second_half);
    }
    else if (split_line[0] == "#=READSEQ" && split_line.size() > 1) {
      
      string read_name = second_half;
      metadata.read_seqs.push_back(read_name);
      if (metadata.adapter_seqs.size() > 0) {
        metadata.adapters_for_reads[read_name] = metadata.adapter_seqs.back();
      }
      
      // Search for corresponding pair
      string pair_name = read_name;
      size_t pos = pair_name.find("_R1");
      if (pos != string::npos) {
        pair_name = pair_name.replace(pos+2, 1, "2");
      }
      else {
        pos = pair_name.find("_R2");
        if (pos != string::npos) {
          pair_name = pair_name.replace(pos+2, 1, "1");
        }
      }
      
      bool found = false;
      for (vector<vector<string> >::iterator it=metadata.reads_by_pair.begin(); it != metadata.reads_by_pair.end(); it++) {
        for (vector<string>::iterator it2=it->begin(); it2 != it->end(); it2++) {
          if (*it2 == pair_name) {
            it->push_back(read_name);
            found = true;
            break;
          }
        }
        if (found) break;
      }
      
      if (!found) {
        metadata.reads_by_pair.push_back(make_vector<string>(read_name));
      }
      
    }
    else if (split_line[0] == "#=ADAPTSEQ" && split_line.size() > 1) {
      metadata.adapter_seqs.push_back(second_half);
    }
    
    else if (split_line[0] == "#=TITLE" && split_line.size() > 1) {
      this->set_title(second_half);
    }
    else if (split_line[0] == "#=TIME" && split_line.size() > 1) {
      metadata.time = from_string<double>(second_half);
    }
    else if (split_line[0] == "#=POPULATION" && split_line.size() > 1) {
      metadata.population = second_half;
      replace(metadata.population.begin(), metadata.population.end(), ' ', '_');
    }
    else if (split_line[0] == "#=TREATMENT" && split_line.size() > 1) {
      metadata.treatment = second_half;
      replace(metadata.treatment.begin(), metadata.treatment.end(), ' ', '_');
    }
    else if (split_line[0] == "#=CLONE" && split_line.size() > 1) {
      metadata.clone = second_half;
      replace(metadata.clone.begin(), metadata.clone.end(), ' ', '_');
    }
    
    // Add every header line to be output
    else {
      if (split_line[0].substr(0, 2) == "#=" && split_line.size() > 1) {                                                                     
        string key = split_line[0].substr(2, split_line[0].size());
        this->add_breseq_data(key, second_half);
        continue;
      } else {
        //Warn if unknown header lines are encountered.
        parse_errors.add_line_error(line_number, whole_line, "Metadata header line not recognized and will be ignored.", false);
      }
    }
  }
  
  /*Error if #=GENOME_DIFF is not found. Genome diff files are required to have
   this header line. */
  
  if (metadata.version.empty()) {
    
    parse_errors.add_line_error(1,"", "No #=GENOME_DIFF XX header line in this file.", true);
    if (!suppress_errors) {
      parse_errors.print_errors();
      exit(1);
    } else { 
      return parse_errors;
    }
  }
  
  //! Step: Handle the diff entries.
  while (in.good()) {
    string line = "";
    breseq::getline(in, line);
    line_number++;
    //Warn if commented out or a possibly blank line is encountered.
    if (line.empty()) {
      continue;
    } else if (line[0] == '#') {
      //printf("Discarding Genome Diff comment file:%s line:\n%s\n", filename.c_str(), line.c_str());
      continue;
    } else if (line.find_first_not_of(' ') == string::npos) {
      continue;
    }
    cDiffEntry de(line, line_number, &parse_errors);
    de.valid_field_variable_types(parse_errors);
    
    // Have to check for unique ids being used at this level
    if (unique_id_used[de._id]) {
      parse_errors.add_line_error(line_number,de.as_string(), "ID for this entry is not unique.", true);
    }
    if (de._type != UNKNOWN) add(de, false); // Don't reassign ids yet    
  }
  
  // Check to be sure all evidence referred to exists

  // Currently don't require this check...
  if (false) {
    for (diff_entry_list_t::iterator it=_entry_list.begin(); it != _entry_list.end(); it++) {
      for (vector<string>::iterator ev_it = (*it)->_evidence.begin(); ev_it != (*it)->_evidence.end(); ev_it++) {
        if (unique_id_used[*ev_it]) {
          parse_errors.add_line_error(line_number,(*it)->as_string(), "Attempt to refer to nonexistent evidence ID.", true);
        }
      }
    }
  }
  
  this->sort_and_check_for_duplicates(&parse_errors);
  
  if (!suppress_errors) {
    parse_errors.print_errors();
    if (parse_errors.fatal() )
    {
      ERROR("Parse errors in loading GenomeDiff File. Not safe to continue");
      exit(1);
    }
  }
  
  // Assign ids to any entries that don't have them (e.g., '.' or '' put in by user)
  diff_entry_list_t the_list = this->get_list();
  for(diff_entry_list_t::iterator it = the_list.begin(); it != the_list.end(); it++ ) {
    if (!cDiffEntry::valid_id((*it)->_id))
        this->assign_unique_id_to_entry(**it);
    //cout << (*it)->as_string() << endl;
  }
  
  return parse_errors;
}
  
// Helper struct for below
typedef struct  {
  cReferenceCoordinate start;
  cReferenceCoordinate end;
  string mutation_id; // of the mutation leading to the requirements
} dr_item;
  
// Checks to see whether seq_ids and coordinates make sense
cFileParseErrors cGenomeDiff::valid_with_reference_sequences(cReferenceSequences& ref_seq, bool suppress_errors)
{
  // For now we do rather generic checking... nothing specific to certain kind of entries
  cFileParseErrors parse_errors(get_file_path());


  // First pass -- check generic things
  for(diff_entry_list_t::iterator it=_entry_list.begin(); it!=_entry_list.end(); ++it) {
    diff_entry_ptr_t& de = *it;
    
    if (de->entry_exists("seq_id")) {
      string seq_id = (*de)["seq_id"];
      if (!ref_seq.seq_id_exists(seq_id)) {
        parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]),de->as_string(), "Seq ID [" + seq_id + "] not found in reference sequence files provided for entry.", true);
        
      } else {
        int32_t valid_start = 1;
        int32_t valid_end = ref_seq[seq_id].get_sequence_length();
      
        // You only have a position if you have a seq_id
        if (de->entry_exists("position")) {
          int32_t position = from_string<int32_t>((*de)["position"]);
          if ((position < valid_start) || (position > valid_end)) {
            parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]),de->as_string(), "Position [" + (*de)["position"] + "] is out of valid range for seq_id [" + to_string<int32_t>(valid_start) + "," + to_string<int32_t>(valid_end) + "] for entry.", true);
          }
          
          /* @JEB needed for circular genomes...
          
          // You only have a size if you have a position
          if (de->entry_exists("size")) {
            
            // All entries with a size (SUB, DEL, INV, AMP, CON) include first base as part of size.
            //    which is why we subtract 1 here
            int32_t test_position = position + from_string<int32_t>((*de)["size"]) - 1;
            
            if ((test_position < valid_start) || (test_position > valid_end)) {  
              parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Position + Size  [" + to_string<uint32_t>(test_position) + "] is out of valid range for seq_id [" + to_string<int32_t>(valid_start) + "," + to_string<int32_t>(valid_end) + "] for entry.", true);
            
            }
          } // end of size
          
          */
          
        } // end of position
      }
    } // end of seq_id
  }
  
  
  // Test that specified mobile elements exist in reference when we will need their sequences
  // including (1) for MOBs and (2) for AMPs with 'mediated' specified.
  
  diff_entry_list_t mut_list = mutation_list();
  
  
  for(diff_entry_list_t::iterator it=mut_list.begin(); it!=mut_list.end(); ++it) {
    diff_entry_ptr_t& de = *it;
    
    string check_repeat_family_name("");
    if (de->_type == MOB) {
      check_repeat_family_name = (*de)[REPEAT_NAME];
    } else if (de->_type == AMP) {
      if (de->entry_exists(MEDIATED))
        check_repeat_family_name = (*de)[MEDIATED];
    }
          
    if (check_repeat_family_name.size() > 0) {
      string found_seq = ref_seq.repeat_family_sequence(check_repeat_family_name, 1, NULL, NULL, NULL, false);
      
      if (found_seq.size() == 0) {
        parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Repeat family name '" + check_repeat_family_name + "' does not exist in reference sequence.", true);
      }
    }
    
  }
  
  // Second pass -- build up a list of coordinates where we require the 'before' or 'within' tag to
  // disambiguate what base has changed. Only applies to MOB and AMP types

  map<string, vector<dr_item> > disambiguate_requirements;

  if (!parse_errors.fatal()) {
    for(diff_entry_list_t::iterator it=mut_list.begin(); it!=mut_list.end(); ++it) {
      diff_entry_ptr_t& de = *it;
      
       if ( (de->_type == MOB) || (de->_type == AMP) || (de->_type == DEL) || (de->_type == SUB) || (de->_type == CON) ) {
        
        dr_item new_dr_item;
        new_dr_item.mutation_id = de->_id;
        new_dr_item.start = de->get_reference_coordinate_start();
        new_dr_item.end = de->get_reference_coordinate_end();
        // @JEB: Note this works properly with respect to negative duplication sizes -> they never overlap anything
        disambiguate_requirements[(*de)[SEQ_ID]].push_back(new_dr_item);
        
      }
    }
  }

  if (!parse_errors.fatal()) {

    // Third pass -- check for specific requirements concerning 'before' and 'within' tags.
    // some code may only be safe if all entries are properly there, which is why this does
    // not happen in the generic main loop or if fatal errors have been encountered so far.
    
    for(diff_entry_list_t::iterator it=mut_list.begin(); it!=mut_list.end(); ++it) {
      diff_entry_ptr_t& de = *it;      

      if (de->is_mutation()) {
        
        // Don't try to have both attributes!! 
        if (de->entry_exists("within") && de->entry_exists("before")) {
          
          // @JEB actually this is OK as long as they are the same within...
          //parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Both 'within' and 'before' attributes found for mutation. Only one of the two is allowed", true);
        }
        
        if (de->entry_exists("within")) {
          
          vector<string> split_within = split((*de)["within"], ":");
          string &within_mutation_id = split_within[0];
          
          // Track down the mutation
          diff_entry_ptr_t within_de = find_by_id(within_mutation_id);
          
          if (within_de.get() == NULL) {
            parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Attempt to put mutation 'within' a mutation with an id that does not exist in file: " + within_mutation_id, true);
          } else {
            
            uint32_t position = from_string<uint32_t>((*de)[POSITION]);
            uint32_t size = de->entry_exists(SIZE) ? from_string<uint32_t>((*de)[SIZE]) : 0;
            uint32_t within_position = from_string<uint32_t>((*within_de)[POSITION]);

            uint32_t valid_start(0);
            uint32_t valid_end(0);
            
            if ((*within_de)[SEQ_ID] != (*de)[SEQ_ID]) {
              parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Attempt to put mutation 'within' a mutation on a different reference sequence id:\n" + within_de->as_string() , true);
            }
            
            if (within_de->_type == AMP) {
              if (split_within.size() != 2) {
                parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Expected AMP field 'within' to be of form 'within=mutation_id:copy_index'. Instead, found: " + (*de)["within"], true);
              }
              
              // check coords to be sure it actually is 'within'
              valid_start = within_position;
              valid_end = valid_start + from_string<uint32_t>((*within_de)[SIZE]) - 1;
              
              // special case for amplified with repeats
              
              if ( within_de->entry_exists("mediated") && within_de->entry_exists("mediated_strand") ) {
                string picked_seq_id;
                cFeatureLocation picked_sequence_feature;
                string mob_region;
                if (within_de->entry_exists("mob_region")) {
                  mob_region = (*within_de)["mob_region"];
                }
                string rep_string = ref_seq.repeat_family_sequence((*within_de)["mediated"], from_string<int16_t>((*within_de)["mediated_strand"]), within_de->entry_exists("mob_region") ? &mob_region : NULL, &picked_seq_id, &picked_sequence_feature);
              
                valid_end += rep_string.size();
              }
              
            } else if (within_de->_type == MOB) {
              
              // check coords to be sure it actually is 'within'
              if (split_within.size() == 1) {
               // it is within the newly inserted sequence, tricky coordinates in play 
                
                string picked_seq_id;
                cFeatureLocation picked_sequence_feature;
                string mob_seq = mob_replace_sequence(ref_seq, *within_de, &picked_seq_id, &picked_sequence_feature);
                
                int32_t ins_end_length = 0;
                if (within_de->entry_exists(INS_END)) ins_end_length = (*within_de)[INS_END].length();
                int32_t ins_start_length = 0;
                if (within_de->entry_exists(INS_START)) ins_start_length = (*within_de)[INS_START].length();
                
                valid_start = within_position + max(0, from_string<int32_t>((*within_de)[DUPLICATION_SIZE]));
                valid_end = valid_start + ins_end_length + ins_start_length + mob_seq.size() - 1;
                
              }
              else if (split_within.size() == 2) {
                  
                
                valid_start = within_position;
                valid_end = valid_start + max(from_string<int32_t>((*within_de)[DUPLICATION_SIZE]), 0) - 1;
                
                int32_t within_copy = from_string<int32_t>(split_within[1]);
                
                if ( (within_copy == 1) && (within_de->entry_exists(INS_START)) ) {
                  valid_end += (*within_de)[INS_START].size();
                }
                if ( (within_copy == 1) && (within_de->entry_exists(INS_START)) ) {
                  valid_end += (*within_de)[INS_START].size();
                }
                if ( (within_copy == 2) && (within_de->entry_exists(INS_END)) ) {
                  valid_end += (*within_de)[INS_END].size();
                }
                
              }
              
            } else if (within_de->_type == INS) {
              if (split_within.size() != 2) {
                parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Expected INS field 'within' to be of form 'within=mutation_id:position'. Instead, found: " + (*de)["within"], true);
              }
              
              // check coords to be sure it actually is 'within'
              
              // test position is in new coordinates that are within this insertion
              uint32_t within_test_position = from_string<int32_t>(split_within[1]);
              uint32_t valid_within_start = 1; // new bases will start after this position
              uint32_t valid_within_end = (*within_de)[NEW_SEQ].size();
              
              if (!( (within_test_position >= valid_within_start) && (within_test_position <= valid_within_end) ) ) {
                parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Position of mutation in 'within=mutation_id:position tag must be within the size of the INS" + to_string(valid_within_start) + "-" + to_string(valid_within_end) + " for mutation that is 'within' this mutation:\n" + within_de->as_string(), true);
              }
              
              // actual position must match base INS occurs after
              valid_start = within_position;
              valid_end = within_position;
              
            } else {
              parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Field 'within' provided for an entry that is not of AMP, MOB, or INS type.", true);
            }
            
            // Last, check the position to be sure that within makes sense
            // this is not as strict as it could be (requiring whole mutation to be inside copy)
            // because we need the leeway for mutations that DEL across an AMP boundary, for instance
            
            // Special case to allow INS before given position for IS-adjacent mutations
            // MOB	27	14,9	REL606	16990	IS150	1	3
            // INS	2625	.	REL606	16989	C	within=27:2
            if (de->_type == INS) valid_start -= 1;
            
            if (!( ((position >= valid_start) && (position <= valid_end)) 
                || ((valid_start >= position) && (valid_start <= position + size - 1)) )) {
              parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Mutation must overlap interval " + to_string(valid_start) + "-" + to_string(valid_end) + " for mutation that is 'within' this mutation:\n" + within_de->as_string(), true);
            }
            
          } // end has 'within' attribute
        } else if (de->entry_exists("before")) {  
          
          uint32_t position = from_string<uint32_t>((*de)[POSITION]);
          uint32_t size = de->entry_exists(SIZE) ? from_string<uint32_t>((*de)[SIZE]) : 0;

          string before_mutation_id = (*de)["before"];
          
          // Track down the mutation
          diff_entry_ptr_t before_de = find_by_id(before_mutation_id);
          
          // Check to be sure the specified entry exists...
          if (before_de.get() == NULL) {
            parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Attempt to put mutation 'before' a mutation with an id that does not exist in file: " + before_mutation_id, true);
          } else {
            // And that it is actually necessary for ordering
 /*
            if (before_de->_type == MOB) {
              
              uint32_t start = from_string<uint32_t>((*before_de)[POSITION]);
              uint32_t end = start + from_string<uint32_t>((*before_de)[DUPLICATION_SIZE]) - 1;
              
              if ((position < start) || (position > end)) {
                parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Position must be >= " + to_string(start) + " and <= " + to_string(end) + " for the 'before' field to have an effect when referring to this mutation:\n" + before_de->as_string(), true);
              }
              
              
            } else if (before_de->_type == AMP) {
              
              uint32_t start = from_string<uint32_t>((*before_de)[POSITION]);
              uint32_t end = start + from_string<uint32_t>((*before_de)[SIZE]) - 1;
              
              if ((position < start) || (position > end)) {
                parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Position must be >= " + to_string(start) + " and <= " + to_string(end) + " for the 'before' field to have an effect when referring to this mutation:\n" + before_de->as_string(), true);
              }              
            
            } else if (before_de->_type == INV) {
              
              uint32_t start = from_string<uint32_t>((*before_de)[POSITION]);
              uint32_t end = start + from_string<uint32_t>((*before_de)[SIZE]) - 1;
              
              if ((position < start) || (position > end)) {
                parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Position must be >= " + to_string(start) + " and <= " + to_string(end) + " for the 'before' field to have an effect when referring to this mutation:\n" + before_de->as_string(), true);
              }
              
            } else {
              parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Field 'before' refers to a mutation that is not of type AMP, MOB, or INV where it will have no effect:\n" + before_de->as_string(), true);
            }
 */
          }
          
        } else {
          // Check to see if we are in a spot that would be ambiguous without a 'within' or 'before' attribute!
          
          vector<dr_item> check_ambiguous = disambiguate_requirements[(*de)[SEQ_ID]];
          cReferenceCoordinate position_start = de->get_reference_coordinate_start();
          cReferenceCoordinate position_end = de->get_reference_coordinate_end();

          for (vector<dr_item>::iterator ita=check_ambiguous.begin(); ita!=check_ambiguous.end(); ita++) {
            
            if ((de->_id != ita->mutation_id) && (position_start >= ita->start) && (position_end <= ita->end)) {
              
              // It's possible that the mutation creating ambiguity is actually 'within' the current mutation
              // in which case the position of the current mutation is not ambiguous
              diff_entry_ptr_t dis_de = find_by_id(ita->mutation_id);
              if (dis_de->entry_exists("within")) {
                
                // @JEB 9-2-2015: We have to allow this for complicated cases...
                
                //vector<string> split_within = split((*dis_de)["within"], ":");
                //if (split_within[0] == de->_id)
                  continue;
              }
              
              parse_errors.add_line_error(from_string<uint32_t>((*de)["_line_number"]), de->as_string(), "Mutation requires 'before' or 'within' field to disambiguate when and how it occurs because it overlaps bases that are duplicated or deleted by another mutation.", true);
            } 
          }
        }
      }
    } // end third pass loop
  } // end already hit fatal error
  
  
  //These are good tests, but they can fail when the mutations are actually 'within' and 'before'
  // Fourth pass loop -- check for bonehead sequence entries
  if (!parse_errors.fatal()) {
    for(diff_entry_list_t::iterator it=_entry_list.begin(); it!=_entry_list.end(); ++it) {
      cDiffEntry& de = **it;
      
      if (de.entry_exists("within"))
        continue;

      if (de._type == RA) {
        
        // Special case for previous way of handling RA bases... we skip these b/c both
        // of the bases used are not the reference base
        if (de.count("error") && (de["error"] == "polymorphic_without_reference_base")) {
          continue;
        }
        
        uint32_t position = from_string<uint32_t>(de[POSITION]);
        uint32_t insert_position = from_string<uint32_t>(de[INSERT_POSITION]);
        string test_ref_base = ((insert_position == 0) ? ref_seq.get_sequence_1(de[SEQ_ID], position, position) : ".");
            
        if (de[REF_BASE] != test_ref_base) {
          parse_errors.add_line_error(from_string<uint32_t>(de["_line_number"]), de.as_string(), "Specified REF_BASE does not match actual reference base (" + test_ref_base + ") at the specified positon.", false);
        }
        
        if (de[REF_BASE] == de[NEW_BASE]) {
          parse_errors.add_line_error(from_string<uint32_t>(de["_line_number"]), de.as_string(), "Specified REF_BASE and NEW_BASE are the same.", false);
        }
      }
      if (de._type == SNP) {
        uint32_t position = from_string<uint32_t>(de[POSITION]);
        if (de[NEW_SEQ] == ref_seq.get_sequence_1(de[SEQ_ID], position, position)) {
          parse_errors.add_line_error(from_string<uint32_t>(de["_line_number"]), de.as_string(), "Specified NEW_SEQ is the same as the reference sequence at the specified position.", false);
        }
      } else if (de._type == SUB) {
        uint32_t position = from_string<uint32_t>(de[POSITION]);
        uint32_t size = from_string<uint32_t>(de[SIZE]);
        if (de[NEW_SEQ] == ref_seq.get_sequence_1(de[SEQ_ID], position, position+size-1)) {
          parse_errors.add_line_error(from_string<uint32_t>(de["_line_number"]), de.as_string(), "Specified NEW_SEQ is the same as the reference sequence at the specified positions.", false);
        }
      }
      
    } // end fourth pass loop
  } // end already hit fatal error
  
  
  if (!suppress_errors) {
    parse_errors.print_errors();
    if (parse_errors.fatal() )
    {
      ERROR("Errors in GenomeDiff File. Not safe to continue");
      exit(1);
    }
  }
  
  return parse_errors;
}


/*! Write this genome diff to a file.
 NOTES:
 1) If you want a diff entry to be commented out(prefix with '#') add the key
 "comment_out" to the diff entry.
 */
void cGenomeDiff::write(const string& filename) {
  string basename = cString(filename).get_base_name();
  string dir = cString(filename).remove_ending(basename);
  if (dir.size()) {
    create_path(dir);
  }
  ofstream os(filename.c_str());
  
  
  //! Step: Header lines.
  /*Always write version tag. It's how we identify it as a genome diff file
   in cGenomeDiff::read(). */
  fprintf(os, "#=GENOME_DIFF\t%s\n", metadata.version.c_str());
  
  if (metadata.title != "") {
    fprintf(os, "#=TITLE\t%s\n", this->get_title().c_str());
  }
  if (metadata.author != "") {
    fprintf(os, "#=AUTHOR\t%s\n", metadata.author.c_str());
  }
  if (metadata.created != "") {
    fprintf(os, "#=CREATED\t%s\n", metadata.created.c_str());
  }
  if (metadata.program != "") {
    fprintf(os, "#=PROGRAM\t%s\n", metadata.program.c_str());
  }
  if (metadata.command != "") {
    fprintf(os, "#=COMMAND\t%s\n", metadata.command.c_str());
  }
  if (metadata.time != -1.0) {
    fprintf(os, "#=TIME\t%s\n", to_string<double>(metadata.time).c_str());
  }
  if (metadata.population != "") {
    fprintf(os, "#=POPULATION\t%s\n", metadata.population.c_str());
  }
  if (metadata.treatment != "") {
    fprintf(os, "#=TREATMENT\t%s\n", metadata.treatment.c_str());
  }
  if (metadata.clone != "") {
    fprintf(os, "#=CLONE\t%s\n", metadata.clone.c_str());
  }
  for (vector<string>::iterator it=metadata.ref_seqs.begin(); it !=metadata.ref_seqs.end(); it++) {
    fprintf(os, "#=REFSEQ\t%s\n", it->c_str());
  }
  for (vector<string>::iterator it=metadata.adapter_seqs.begin(); it !=metadata.adapter_seqs.end(); it++) {
    fprintf(os, "#=ADAPTSEQ\t%s\n", it->c_str());
  }
  for (vector<string>::iterator it=metadata.read_seqs.begin(); it !=metadata.read_seqs.end(); it++) {
    fprintf(os, "#=READSEQ\t%s\n", it->c_str());
  }

  if (!metadata.breseq_data.empty()) {
    for (map<key_t,string>::iterator it = metadata.breseq_data.begin();
         it != metadata.breseq_data.end(); it ++) {
      fprintf(os, "#=%s\t%s\n", it->first.c_str(), it->second.c_str());
    }
  }
  
  // sort
  cFileParseErrors fpe;
  this->sort_and_check_for_duplicates(&fpe);
  
  // @JEB: "comment_out" tag is legacy used internally for filtering where
  // deletion from the list should really be used.
  
  for(diff_entry_list_t::iterator it=_entry_list.begin(); it!=_entry_list.end(); ++it) {
    if (!(*it)->entry_exists("comment_out")) {
      fprintf(os, "%s\n", (**it).as_string().c_str());
    } else {
      (*it)->erase("comment_out");
      fprintf(os, "#%s\n", (**it).as_string().c_str());
    }
  }
  os.close();

  if (fpe.fatal()) {
    fpe.print_errors();
    ERROR("Fatal formatting error in GD file being written: " + filename);
  }

}
  
/*! Find the next unused unique id.
 */
uint32_t cGenomeDiff::new_unique_id()
{ 
  uint32_t assigned_id = ++_unique_id_counter;
  
  while (unique_id_used.count(to_string<uint32_t>(assigned_id)))
  {
    assigned_id++;
  }
  return assigned_id;
}

/*! Be sure we have a valid unique id
 */
void cGenomeDiff::assign_unique_id_to_entry(cDiffEntry &de) {
  
  //Get a new valid id if needed
  de._id = to_string<int32_t>(new_unique_id());
  
  // Record that we used this
  unique_id_used[de._id] = true;
}
  
/*! Add evidence to this genome diff.
 */
diff_entry_ptr_t cGenomeDiff::add(const cDiffEntry& item, bool reassign_id) {
  
  ASSERT(item._type != UNKNOWN, "Tried to add item of type UNKNOWN to Genome Diff file.");
  
 /* This is SLOW, now we only check this when specifically asking about it
  // check to see if we are adding a duplicate item
  // -- we don't allow, give a warning, and return existing item
  for (diff_entry_list_t::iterator it = _entry_list.begin(); it != _entry_list.end(); it++) {
    
    // don't do this on validation like NOTEs
    if ((*it)->is_validation()) {
      continue;
    }
    
    if (**it == item) {
      WARN("Ignored attempt to add GD item:\n" + item.as_string() + "\nwhich is a duplicate of existing item:\n" + (*it)->as_string());
      return *it;
    }
  }
*/
  
  // allocating counted_ptr takes care of disposal
  cDiffEntry* diff_entry_copy = new cDiffEntry(item);
  diff_entry_ptr_t added_item(diff_entry_copy);
  _entry_list.push_back(added_item);
  

  // Record that we have used this id
  if (reassign_id) {
    assign_unique_id_to_entry(*added_item);
  }
  
  if (cDiffEntry::valid_id(added_item->_id)) {
    unique_id_used[added_item->_id] = true;
  }

  //cout << added_item->as_string() << endl;  
  return added_item;
}
  
void cGenomeDiff::remove_group(cGenomeDiff::group group)
{
  diff_entry_list_t::iterator it = _entry_list.begin();
  
  bool advance_it(true);
  while (it != _entry_list.end()) {
    advance_it = true;
    
    if ((group == cGenomeDiff::MUTATIONS) && (*it)->is_mutation() ){
      it = remove(it);
      advance_it = false;
    } else if ((group == cGenomeDiff::EVIDENCE) && (*it)->is_evidence() ){
      it = remove(it);
      advance_it = false;
    } else if ((group == cGenomeDiff::VALIDATION) && (*it)->is_validation() ){
      it = remove(it);
      advance_it = false;
    }
    
    if (advance_it) ++it;
  }
}
  
void cGenomeDiff::remove_type(gd_entry_type _type)
{
  diff_entry_list_t::iterator it = _entry_list.begin();
  
  bool advance_it(true);
  while (it != _entry_list.end()) {
    advance_it = true;
    
    if ((*it)->_type ==  _type){
      it = remove(it);
      advance_it = false;
    }
    
    if (advance_it) ++it;
  }
}
  
void cGenomeDiff::remove_all_but_mutations_and_unknown()
{
  remove_group(cGenomeDiff::VALIDATION);
  remove_type(RA);
  remove_type(MC);
  remove_type(CN);
  remove_type(JC);
}

/*! Given an id return the entry if it exists. NULL otherwise.
 */ 

diff_entry_ptr_t cGenomeDiff::find_by_id(string _id)
{
  for (diff_entry_list_t::iterator itr_diff_entry = _entry_list.begin();
       itr_diff_entry != _entry_list.end(); itr_diff_entry++)
  {    
    if ( (*itr_diff_entry)->_id == _id)
      return *itr_diff_entry;
  }
  return diff_entry_ptr_t(NULL);
}


  
  
/*! Given a list of types, search and return the cDiffEntry's within diff_entry_list_t whose 
 * _type parameter matches one of those within input types. 
 */ 
diff_entry_list_t cGenomeDiff::get_list(const vector<gd_entry_type>& types)
{
  // default is to have to types
  if (types.size() == 0)
    return _entry_list;
  
  diff_entry_list_t return_list;
  
  for (diff_entry_list_t::iterator itr_diff_entry = _entry_list.begin();
       itr_diff_entry != _entry_list.end(); itr_diff_entry++)
  {
    for (vector<gd_entry_type>::const_iterator requested_type = types.begin();
         requested_type != types.end(); requested_type++)
    {
      if((*itr_diff_entry)->_type == *requested_type)
        return_list.push_back(*itr_diff_entry);
    }
  }
  
  return return_list;
}


diff_entry_list_t cGenomeDiff::show_list(const vector<gd_entry_type>& types)
{
  diff_entry_list_t ret_list = get_list(types);
  ret_list.remove_if(cDiffEntry::fields_exist(make_vector<diff_entry_key_t>("deleted")));
  ret_list.remove_if(cDiffEntry::fields_exist(make_vector<diff_entry_key_t>("no_show")));
  return ret_list;
}
  
  
/*! Return all cDiffEntrys within _entry_list whose _id matches one
 * of those within input's item._evidence
 */ 
diff_entry_list_t cGenomeDiff::mutation_evidence_list(const cDiffEntry& item)
{
  diff_entry_list_t return_list;
  
  //return diff_entries with matching evidence
  for (vector<string>::const_iterator itr_i = item._evidence.begin(); itr_i != item._evidence.end(); itr_i ++) 
  {  
    const string& evidence = *itr_i;
    
    for (diff_entry_list_t::iterator itr_j = _entry_list.begin(); itr_j != _entry_list.end(); itr_j ++)
    {  
      cDiffEntry& entry = **itr_j;
      
      if (entry._id == evidence)
        return_list.push_back(*itr_j);
    }   
  }
  return return_list;
}
  
// return items with types that are 3 characters long
diff_entry_list_t cGenomeDiff::mutation_list()
{
  diff_entry_list_t::iterator it = _entry_list.begin();
  while (it != _entry_list.end()) {
    if (!(**it).is_mutation()) {
      break;
    } else {
      ++it;
    }
  }
  
  return diff_entry_list_t(_entry_list.begin(), it);
}

// return items with types that are 2 characters long
diff_entry_list_t cGenomeDiff::evidence_list()
{
  diff_entry_list_t mut_list;
  
  for(diff_entry_list_t::iterator itr = _entry_list.begin();
      itr != _entry_list.end(); itr ++) {
    cDiffEntry& item = **itr;
    if(item.is_evidence()) {
      mut_list.push_back(*itr);
    }
  }
  
  return mut_list;
}

// return items with types that are 4 characters long
diff_entry_list_t cGenomeDiff::validation_list()
{
  this->sort();
  diff_entry_list_t::reverse_iterator it = _entry_list.rend();
  while (it != _entry_list.rbegin()) {
    if (!(**it).is_validation()) {
      --it;
      break;
    } else {
      ++it; 
    }
  }
  return diff_entry_list_t(it.base(), _entry_list.end());
}

//! Removes all GD entries that aren't used as evidence.
void cGenomeDiff::filter_not_used_as_evidence(bool verbose)
{
  // Yes, I know the bool is useless.
  map<string,bool> used_evidence;
  
  diff_entry_list_t muts = this->mutation_list();
  //Iterate through all mutations
  for (diff_entry_list_t::iterator it = muts.begin(); it != muts.end(); it++)
  {    
    //For every piece of evidence this entry has
    for(uint32_t iter = 0; iter < (**it)._evidence.size(); iter++)
    {
      //Each piece of evidence will only get assigned to the map once.
      used_evidence[(**it)._evidence[iter]] = true;
    }
  }
  
  // We will be erasing inside the it loop.  This is to keep
  // track of whether or not we should iterate to the next element.
  bool it_iterate = true;
  
  //Iterate through all entries
  for (diff_entry_list_t::iterator it = _entry_list.begin(); it != _entry_list.end(); )
  {
    bool it_iterate = true;
    
    //Is this ID in our map of used_evidence?
    if(!(used_evidence.count((**it)._id)) && (**it).is_evidence())
    {
      //Inform the user
      if(verbose){cout << "NOT USED AS EVIDENCE: " << (**it)._id << endl;}
      
      //Remove this entry from the list.
      it = _entry_list.erase(it);
      
      //We just removed the current feauture, do not iterate.
      it_iterate = false;
    }
    
    // Iterate it ONLY if we haven't erased something.
    if(it_iterate)it++;
  }
}

/*-----------------------------------------------------------------------------
 * returns entries NOT used as evidence by other entries. 
 *
 *-----------------------------------------------------------------------------*/
diff_entry_list_t cGenomeDiff::filter_used_as_evidence(const diff_entry_list_t& input)
{
  // first we make a map with everything used as evidence by any entry in the entire genome diff
  map<string,bool> used_as_evidence;
  for (diff_entry_list_t::const_iterator it = _entry_list.begin(); it != _entry_list.end(); it++)
  {
    const diff_entry_ptr_t& de = *it;
    for (vector<string>::const_iterator ev_it = de->_evidence.begin(); ev_it != de->_evidence.end(); ev_it++) 
    {  
      used_as_evidence[*ev_it] = true;
    }   
  }
  
  // then construct a list of all items in input with ids not in this map
  diff_entry_list_t return_list;
  for (diff_entry_list_t::const_iterator it = input.begin(); it != input.end(); it++)
  {
    const diff_entry_ptr_t& de = *it;
    if ( !used_as_evidence.count(de->_id) )
      return_list.push_back(de);
  }
  
  return return_list;
}
  
void cGenomeDiff::strip_to_spec()
{
  for (diff_entry_list_t::iterator it = _entry_list.begin(); it != _entry_list.end(); it++) {
    (*it)->strip_to_spec();
  }
}

// Helper function for mutation_unknown and mutation_deleted
// --> should optimize for nonoverlapping intervals
bool cGenomeDiff::mutation_in_entry_of_type(cDiffEntry mut, const gd_entry_type type)
{
  cReferenceCoordinate start = mut.get_reference_coordinate_start();
  cReferenceCoordinate end = mut.get_reference_coordinate_end();
  
  diff_entry_list_t check_list = get_list(make_vector<gd_entry_type>(type));
  
  for (diff_entry_list_t::iterator itr = check_list.begin(); itr != check_list.end(); itr++) {
    
    cDiffEntry de(**itr);
    if (mut[SEQ_ID] != de[SEQ_ID])
      continue;
    if ( (start >= de.get_reference_coordinate_start()) && (end <= de.get_reference_coordinate_end()) ) {
      return true;
    }
  }
  return false;
}

//! Subtract mutations
//Note: Preserves evidence in the file being subtracted from
void cGenomeDiff::set_subtract(cGenomeDiff& gd, bool phylogeny_id_aware, bool verbose)
{
  (void)verbose; //unused

  // Temporarily delete 'phylogeny_id' if we are not phylogeny aware
  if (!phylogeny_id_aware) {
    for (diff_entry_list_t::iterator it_new = gd._entry_list.begin(); it_new != gd._entry_list.end(); it_new++) {
      
      // Save this info in field that won't affect comparisons
      if ((*it_new)->entry_exists("phylogeny_id")) {
        (**it_new)["_phylogeny_id"] = (**it_new)["phylogeny_id"];
      }
      if ((*it_new)->entry_exists("population_id")) {
        (**it_new)["_population_id"] = (**it_new)["population_id"];
      }
      
      (*it_new)->erase("phylogeny_id");
      (*it_new)->erase("population_id");
    }
  }
  
  set<cDiffEntry> seen;
  diff_entry_list_t muts = gd.mutation_list();
  for (diff_entry_list_t::iterator it = muts.begin(); it != muts.end(); ++it) {
    seen.insert(**it);
  }
  
  // We will be erasing inside the it loop.  This is to keep
  // track of whether or not we should iterate to the next element.
  bool it_advance = true;
  
  //Iterate through all the entries
  for (diff_entry_list_t::iterator it = _entry_list.begin(); it != _entry_list.end(); )
  {
    it_advance = true;
    //The current entry we're looking at
    cDiffEntry& entry = **it;
    
    //if (verbose) cout << entry << endl;
    
    if (!phylogeny_id_aware) {
      // Save this info in field that won't affect comparisons
      if (entry.entry_exists("phylogeny_id")) {
        entry["_phylogeny_id"] = entry["phylogeny_id"];
      }
      if (entry.entry_exists("population_id")) {
        entry["_population_id"] = entry["population_id"];
      }
    }
    
    entry.erase("phylogeny_id");
    entry.erase("population_id");
    
    //Subtract mutations that we've seen
    if(entry.is_mutation() && seen.count(entry)) {
      it = _entry_list.erase(it);
      it_advance = false;
    } else if (!phylogeny_id_aware) {
      if (entry.entry_exists("_phylogeny_id")) {
        entry["phylogeny_id"] = entry["_phylogeny_id"];
      }
      if (entry.entry_exists("_population_id")) {
        entry["population_id"] = entry["_population_id"];
      }
    }
    
    // Iterate it ONLY if we haven't erased something.
    if(it_advance)it++;
  }
  
  // Restore fields that we hid if we are not phylogeny aware
  if (!phylogeny_id_aware) {
    for (diff_entry_list_t::iterator it_new = _entry_list.begin(); it_new != _entry_list.end(); it_new++) {
      
      if ((*it_new)->entry_exists("_phylogeny_id")) {
        (**it_new)["phylogeny_id"] = (**it_new)["_phylogeny_id"];
      }
      if ((*it_new)->entry_exists("_population_id")) {
        (**it_new)["population_id"] = (**it_new)["_population_id"];
      }
    }
  }
  
  reassign_unique_ids();
}
 
  
// Strips files of all items that are not in common AND all non-mutation items
void cGenomeDiff::set_intersect(cGenomeDiff &gd, bool verbose)
{
  (void)verbose; //unused
  
  set<cDiffEntry> seen;
  diff_entry_list_t muts = gd.mutation_list();
  
  for (diff_entry_list_t::iterator it = muts.begin(); it != muts.end(); ++it) {
    seen.insert(**it);
  }

  this->remove_group(cGenomeDiff::EVIDENCE);
  this->remove_group(cGenomeDiff::VALIDATION);
  
  diff_entry_list_t::iterator it = _entry_list.begin();
  
  //Handle mutations
  bool it_advance(true);
  
  while (it != _entry_list.end()) {
    it_advance = true;
    
    if ( !seen.count(**it) ) {
      it = _entry_list.erase(it);
      it_advance = false;
    } else {
      (*it)->_evidence.clear(); // clear the evidence
    }
    
    // Iterate it ONLY if we haven't erased something.
    if(it_advance) it++;
  }
  
  // Re-number entries
  reassign_unique_ids();
}
  
// Merged mutations AND all non-mutation items
void cGenomeDiff::set_union(cGenomeDiff& gd, bool evidence_mode, bool phylogeny_aware, bool verbose)
{
  (void)verbose; //unused

  if (!evidence_mode)
    this->remove_group(cGenomeDiff::EVIDENCE);
  else
    this->remove_group(cGenomeDiff::MUTATIONS);
  this->remove_group(cGenomeDiff::VALIDATION);
  
  // Merge and clear away all evidence reference
  merge(gd, true, true, phylogeny_aware, verbose);
  
  if (!evidence_mode)
    this->remove_group(cGenomeDiff::EVIDENCE);
  else
    this->remove_group(cGenomeDiff::MUTATIONS);
  this->remove_group(cGenomeDiff::VALIDATION);
}

// Keeps only one copy when equivalent entries are encountered
void cGenomeDiff::unique()
{
  bool (*comp_fn) (const diff_entry_ptr_t&, const diff_entry_ptr_t&) = diff_entry_ptr_sort;
  typedef set<diff_entry_ptr_t, bool(*)(const diff_entry_ptr_t&, const diff_entry_ptr_t&)> diff_entry_set_t;
  //Filter entries.
  diff_entry_set_t seen(comp_fn);
  //Store pointers to mutations.
  map<string, vector<diff_entry_ptr_t> > keep_ids;
  //Store ids of evidence to erase.
  set<string> erase_ids;
  
  this->sort();
  diff_entry_list_t::iterator it = _entry_list.begin();
  while (it != _entry_list.end()) {
    if (!(**it).is_mutation()) break;
    
    const vector<string>& ids = (**it)._evidence;
    uint32_t n = ids.size();
    
    //Is mutation unique?
    //Case: true.
    if (seen.insert(*it).second) { 
      for (uint32_t i = 0; i < n; ++i) {
        keep_ids[ids[i]].push_back(*it);
      }
      ++it;
    } 
    //Case: false.
    else { 
      for (uint32_t i = 0; i < n; ++i) {
        erase_ids.insert(ids[i]);
      }
      it = _entry_list.erase(it);
    }
  }
  
  seen.clear(); //Re-use to filter the evidence.
  while (it != _entry_list.end()) {
    //Keep this evidence?
    //Case: unkown.
    if (keep_ids.count((**it)._id) && erase_ids.count((**it)._id)) {
      //Is evidence unique?
      //Case: true.
      if (seen.insert(*it).second) {
        ++it;
      } 
      //Case: false.
      else {
        it = _entry_list.erase(it);
        //Update mutations that may have been using this id.
        for (uint32_t i = 0; i < keep_ids[(**it)._id].size(); ++it) {
          vector<string>* evidence = &(*keep_ids[(**it)._id][i])._evidence;
          vector<string>::iterator jt = std::remove(evidence->begin(), evidence->end(), (**it)._id);
          //evidence->erase(jt);
        }
      }
    } 
    //Case: false.
    else if (!keep_ids.count((**it)._id) && erase_ids.count((**it)._id)) {
      it = _entry_list.erase(it);
    } 
    //Case: false.
    else if (!keep_ids.count((**it)._id) && !erase_ids.count((**it)._id)) {
      stringstream ss;
      ss << "\tRemoving [entry]:\t" << **it << endl;
      ss << "\tfrom [file]:\t" << this->get_file_path() << endl;
      ss << "\tbecause no mutation referenced it's ID." << endl; 
      WARN(ss.str());
      it = _entry_list.erase(it);
    }
    
    //Case: true.
    else {
      ++it;
    }
  }
  
  return;
}



//! Merge GenomeDiff information using gd_new as potential new info.
//  Evidence IDs that are not unique are given new IDs.  Mutations
//  that refer to this evidence have their evidence updated as well.
//
//  bool unique:  TRUE will NOT merge entries that match existing entries.
//                FALSE WILL merge entries that match existing entries.
//
//  bool new_id:  TRUE will assign all new entries the lowest available ID.
//                FALSE will allow all entries to retain their IDs if they are unique.
//
// There are some complicated cases that merge just cannot accommodate:
//
// 1) The difference between a mutation happening before an AMP (thus in all copies)
//    and after an AMP (thus in only one copy) at the same position.
//
void cGenomeDiff::merge(cGenomeDiff& merge_gd, bool unique, bool new_id, bool phylogeny_id_aware, bool verbose)
{
  uint32_t old_unique_ids = unique_id_used.size();
  
  cGenomeDiff new_gd;
  
  // Add population info if we are phylogeny aware, we do this outside the
  // loop because changing it shouldn't be able to disambiguate any entries
  // unlike phylogeny_id and population_id when in !phylogeny_id_aware mode
  if (phylogeny_id_aware) {
    
    diff_entry_list_t mut_list = merge_gd.mutation_list();
    for (diff_entry_list_t::iterator it_new = mut_list.begin(); it_new != mut_list.end(); it_new++) {
      if (merge_gd.metadata.population.size() != 0) {
        (**it_new)["population_id"] = merge_gd.metadata.population;
      }
    }
  }
  
  //Iterate through all the potential new entries
  for (diff_entry_list_t::iterator it_new = merge_gd._entry_list.begin(); it_new != merge_gd._entry_list.end(); it_new++)
  {
    //The current potential new entry we're looking at
  
    // Make a copy of the entry so that we are not changing
    // the original GD at all!
    
    // Allocating diff_entry_ptr_t takes care of disposal
    cDiffEntry* diff_entry_copy = new cDiffEntry(**it_new);
    diff_entry_ptr_t added_item(diff_entry_copy);
    
    cDiffEntry& entry_new = *diff_entry_copy;

    // Ditch the extra baggage, but only in the copy
    if (!phylogeny_id_aware) {
      
      // Save this info in field that won't affect comparisons
      if (entry_new.entry_exists("phylogeny_id")) {
        entry_new["_phylogeny_id"] = entry_new["phylogeny_id"];
      }
      if (entry_new.entry_exists("population_id")) {
        entry_new["_population_id"] = entry_new["population_id"];
      }
      
      entry_new.erase("phylogeny_id");
      entry_new.erase("population_id");
    }
    
    bool new_entry = true;
    
    //Iterate through all the entries in the current list.
    for (diff_entry_list_t::iterator it_cur = _entry_list.begin(); it_cur != _entry_list.end() && unique; it_cur++)
    {
      //The current entry we're looking at
      cDiffEntry& entry = **it_cur;
      
      //Does the new entry match the current entry?
      if(entry == entry_new)
      {
        //Existing matching entry found, this is not new
        new_entry = false;
        break;
      }
    }
    
    //We definitely have a new entry
    if(new_entry)
    {      
      //Add the new entry to the existing list
      new_gd.add(entry_new, new_id);
      
      //Notify user of new entry
      //if(verbose)cout << "\tNEW ENTRY\t" << entry_new.as_string() << "\t" << merge_gd.get_file_path() << endl;
    }
  }
  
  //Iterate through all the entries in the new list.
  //This is where we update the evidence IDs for mutations.
  for (diff_entry_list_t::iterator it = new_gd._entry_list.begin(); it != new_gd._entry_list.end(); it++)
  {
    // @JEB: optimization: we don't need to do this for evidence items.
    if ( !(**it).is_mutation() ) continue;
    
    //For every piece of evidence this entry has
    for(int32_t iter = 0; iter < (int32_t)(**it)._evidence.size(); iter++)
    {
      bool found_match = false;
      
      //Iterate through all the potential new entries
      for (diff_entry_list_t::iterator it_new = new_gd._entry_list.begin(); it_new != new_gd._entry_list.end(); it_new++)
      {            
        //Does this evidence ID match an ID in the old list?
        if((**it)._evidence[iter] == (**it_new)._id && !found_match)
        {
          //Iterate through all the current entries
          for (diff_entry_list_t::iterator it_cur =_entry_list.begin(); it_cur != _entry_list.end(); it_cur++)
          {
            //Does the new entry match the current entry?
            if((**it_cur) == (**it_new))
            {
              //Notify user of the update
              //if(verbose)cout << "\tID: " << (**it)._id  << "\tEVIDENCE\t" << (**it)._evidence[iter] << " >> " << (**it_cur)._id << endl;
              
              //Change the evidence ID to it's new ID in the new updated list
              (**it)._evidence[iter] = (**it_cur)._id;
              found_match = true;  
              break;
            }
          }
        }
      }
      
      // If we've gone through all the lists, and can't find the evidence
      // then remove the evidence entry completely, as it matches to nothing.
      if(!found_match)
      {
        //Notify user of the update
        //if(verbose)cout << "\tID: " << (**it)._id  << "\tEVIDENCE  \t" << (**it)._evidence[iter] << " >> REMOVED" << endl;
        (**it)._evidence.erase((**it)._evidence.begin() + iter--);
      }        
    }
  }

  // Add the ones from the new list to the current list
  for (diff_entry_list_t::iterator it=new_gd._entry_list.begin(); it!= new_gd._entry_list.end(); it++) {
    (**it)["_just_added"] = "1";
    
    // Restore fields that we hid if we are not phylogeny aware
    // this is necessary now so that we will match the proper first entries
    if (!phylogeny_id_aware) {
      if ((**it).entry_exists("_phylogeny_id")) {
        (**it)["phylogeny_id"] = (**it)["_phylogeny_id"];
      }
      if ((**it).entry_exists("_population_id")) {
        (**it)["population_id"] = (**it)["_population_id"];
      }
    }
    
    add(**it);
  }
  
  // Fix optional fields that refer to mutation IDs
  // Must be done after adding entries, because a before= or within= could have existed in the original GD (so not in the added list)
  for (diff_entry_list_t::iterator it=_entry_list.begin(); it!= _entry_list.end(); it++) {
    
    cDiffEntry& mut = **it;
    
    if (!mut.is_mutation()) continue;
    if (!mut.entry_exists("_just_added")) continue;
    
    // Very important to erase for multiple merges!
    mut.erase("_just_added");
    
    for (vector<string>::const_iterator key_it=gd_keys_with_ids.begin(); key_it!= gd_keys_with_ids.end(); key_it++) {
      if (mut.entry_exists(*key_it)) {
        
        string value = mut[*key_it];
        // Replace first value = mutation_id with substituted id
        vector<string> split_value = split(value, ":");
        
        // looking for original entry
        // ---> note that this is unchanged for phylogeny mode
        diff_entry_ptr_t looking_for = merge_gd.find_by_id(split_value[0]);
        
        if (looking_for.get() == NULL) continue;
        
        //cout << "Looking for: " << looking_for->as_string() << endl;
        
        // iterate through all items to find the match
        bool found_match = false;
        
        //Iterate through all the current entries to find the match
        for (diff_entry_list_t::iterator it_cur =_entry_list.begin(); it_cur != _entry_list.end(); it_cur++) {
          if (**it_cur == *looking_for) {
            //cout << "Found: " << (*it_cur)->as_string() << endl;

            split_value[0] = to_string((**it_cur)._id);
            found_match = true;
            break;
          }
        }
        
        if (found_match) {
        
          if (verbose) {
            cout << "Mutation:" << endl << mut.as_string() << endl;
          }
          mut[*key_it] = join(split_value, ":");

          if (verbose) {
            cout << "  Reassigned " << *key_it << "=" << value << " to " << mut[*key_it] << endl;
            cout << "Mutation:" << endl << mut.as_string() << endl;
          }
          
        } else {
          WARN("Did not find entry referred to by key '" + *key_it + "' for mutation\n" + mut.as_string() + "\nThis key will be deleted.");
          mut.erase(*key_it);
        }
      }
    }
  }
  
  // @JEB: Added 2016-02-06
  // It is now safe to delete (in fact we should) the phylogeny information?
  /*
  if (!phylogeny_id_aware) {
    for (diff_entry_list_t::iterator it=_entry_list.begin(); it!= _entry_list.end(); it++) {
      (*it)->erase("phylogeny_id");
      (*it)->erase("population_id");
    }
  }
   */
  
  //Notify user of the update
  if(verbose)cout << "\tMERGE DONE - " << merge_gd.get_file_path() << endl;
  
}
  

void cGenomeDiff::fast_merge(const cGenomeDiff& gd)
{  
  diff_entry_list_t gd_list = gd.get_const_list();
  for(diff_entry_list_t::const_iterator it=gd_list.begin(); it!= gd_list.end(); it++) {
    this->add(**it);
  }
}
  
// This function is for simplifying the numbering
void cGenomeDiff::reassign_unique_ids()
{
  bool verbose = false;
  this->sort();
  
  //Handle mutations.
  _unique_id_counter = 0;
  
  // Need to map out what we reassigned (as strings!) in order to update
  // in various other fields that refer to these
  
  map<string,string> mutation_id_reassignments;
  
  // Need to know what mutation's evidence to update when
  // the evidence is renumbered
  map<string, vector<diff_entry_ptr_t> > id_table;
  
  for (diff_entry_list_t::iterator it=_entry_list.begin(); it!= _entry_list.end(); it++) {
    if (!(**it).is_mutation()) continue;
    
    string old_id = (**it)._id;
    (**it)._id = to_string(++_unique_id_counter);
    mutation_id_reassignments[old_id] = (**it)._id;
    
    // Keep pointers back to this mutation based on evidence ids
    for (uint32_t i = 0; i < (**it)._evidence.size(); ++i) {
      id_table[(**it)._evidence[i]].push_back(*it);
    }
    (**it)._evidence.clear();
  }
  
  //Handle the evidence and validation (any non-mutation)
  for (diff_entry_list_t::iterator it=_entry_list.begin(); it!= _entry_list.end(); it++) {
    if ((**it).is_mutation()) continue;
    
    string new_id = to_string(++_unique_id_counter);
    
    if (id_table.count((**it)._id)) {
      for (uint32_t i = 0; i < id_table[(**it)._id].size(); ++i) {
        id_table[(**it)._id][i]->_evidence.push_back(new_id);
      }
    }
    
    (**it)._evidence.clear(); // These entries should not have any evidence
    (**it)._id = new_id;
  }
  
  // Handle updating these tags which may refer to evidence
  // before=, within=, 
  
  for (diff_entry_list_t::iterator it=_entry_list.begin(); it!= _entry_list.end(); it++) {
    if (!(**it).is_mutation()) continue;    
    
    cDiffEntry& mut = **it;
    for (vector<string>::const_iterator key_it=gd_keys_with_ids.begin(); key_it!= gd_keys_with_ids.end(); key_it++) {
      if (mut.entry_exists(*key_it)) {
        
        string value = mut[*key_it];
        // Replace first value = mutation_id with substituted id
        vector<string> split_value = split(value, ":");
        if(!mutation_id_reassignments.count(split_value[0])) {
          WARN("Mutation that this mutation was 'before' or 'within' has been removed.\n" + mut.as_string());
          mut.erase(*key_it);
        } else {
          split_value[0] = mutation_id_reassignments[split_value[0]];
          
          if (verbose) {
            cout << "Mutation:" << endl << mut.as_string() << endl;
          }
          mut[*key_it] = join(split_value, ":");
          if (verbose) {
            cout << "Reassigned " << *key_it << "=" << split_value[0] << " to " << mut[*key_it] << endl;
            cout << "New Mutation:" << endl << mut.as_string() << endl;
          }
        }
      }
    }
  }
}

/*! Write this genome diff to a file.
 */
 
// This version of sort does not discriminate between additional optional fields!!
bool cGenomeDiff::diff_entry_ptr_compare_sort(const diff_entry_ptr_t& a, const diff_entry_ptr_t& b) {
  int32_t compare_result = cDiffEntry::compare(*a,*b);

  if (compare_result < 0) {
    return true;
  } else if (compare_result > 0) {
    return false;
  }
  return false;
}
  
bool cGenomeDiff::diff_entry_ptr_sort(const diff_entry_ptr_t& a, const diff_entry_ptr_t& b) {

  int32_t compare_result = cDiffEntry::compare(*a,*b);
  
  if (compare_result < 0) {
    return true;
  } else if (compare_result > 0) {
    return false;
  }
  
  // @JEB The sorts below should not be necessary if every item is unique
  // DEPRECATE THEM WHEN WE MOVE TO STORING AS A SET
  // ** last sort by id
  
  // First as numbers
  uint32_t a_sort_id = from_string<uint32_t>(a->_id);
  uint32_t b_sort_id = from_string<uint32_t>(b->_id);
  
  if (a_sort_id < b_sort_id) {
    return true;
  } else if (a_sort_id > b_sort_id) {
    return false;
  } 
  
  // Then try as string 
  if (a->_id < b->_id) {
    return true;
  } else if (a->_id < b->_id) {
    return false;
  } 
  
  //Finally try the evidence fields
  if (join(a->_evidence, ",") < join(b->_evidence, ",")) {
    return true;
  } else if (join(a->_evidence, ",") > join(b->_evidence, ",")) {
    return false;
  }
  
  ERROR("Identical diff entry items found in sort:\n1>>\n" + a->as_string() + "\n2>>\n" + b->as_string() + "\n" );
  return false;
}

  /*! Helper struct and function for apply sort
   */

  
//Correctly accounts for 'before' and 'within' tags
void cGenomeDiff::sort_apply_order() {
  
  // Remove all items with tags into these special lists
  diff_entry_list_t add_back_list;
  
  diff_entry_list_t::iterator it = _entry_list.begin();
  while (it != _entry_list.end()) {
    bool advance_it = true;
    
    cDiffEntry& mut = **it;
    if (mut.entry_exists("before") || mut.entry_exists("within")) {
      add_back_list.push_back(*it);
      it = _entry_list.erase(it);
      advance_it = false;
    }
    
    if (advance_it) it++;
  }
  
  // Sort the normal entries
  this->sort();
  
  // Now add back the special ones (this has to be iterative until list is cleared)
  // because one may before another that is before another, etc.
  
  while(add_back_list.size() > 0) {
    
    diff_entry_list_t::iterator it = add_back_list.begin();

    size_t old_size = add_back_list.size();
    
    while (it != add_back_list.end()) {
      
      bool advance_it = true;
      cDiffEntry& mut = **it;
      
      if (mut.entry_exists("before")) {
        
        vector<string> split_value = split(mut["before"], ":");
        string mutation_id = split_value[0];
        
        for(diff_entry_list_t::iterator find_it=_entry_list.begin(); find_it!=_entry_list.end(); find_it++)
        {
          cDiffEntry& find_mut = **find_it;
          if (find_mut._id == mutation_id) {
            
            _entry_list.insert(find_it, *it);
            it = add_back_list.erase(it);
            advance_it = false;
            break;
          }
        }
        
      } else if (mut.entry_exists("within")) {
      
        vector<string> split_value = split(mut["within"], ":");
        string mutation_id = split_value[0];
        
        for(diff_entry_list_t::iterator find_it=_entry_list.begin(); find_it!=_entry_list.end(); find_it++)
        {
          cDiffEntry& find_mut = **find_it;
          if (find_mut._id == mutation_id) {
            
            find_it++;  // we are inserting after this item
            _entry_list.insert(find_it, *it);
            it = add_back_list.erase(it);
            advance_it = false;
            break;
          }
        }
        
      }
      
      if (advance_it) it++;
    }
    
    // We're in trouble if no entries got added = infinite loop due to circular references
    if (add_back_list.size() == old_size) {
      
      string error_string = "Could not be sorted due to invalid 'before' or 'within' constraint in mutation(s):";
      for(diff_entry_list_t::iterator it=add_back_list.begin(); it!=add_back_list.end(); it++) {
        error_string += "\n" + (*it)->as_string();
      }
      ERROR(error_string);
    }
  }
}
  
  
void cGenomeDiff::sort_and_check_for_duplicates(cFileParseErrors* file_parse_errors) {
  
  this->sort();
  
  diff_entry_list_t::iterator it2;
  
  
  for (diff_entry_list_t::iterator it1 = this->_entry_list.begin(); it1 != this->_entry_list.end(); it1++) {
    
    if ((*it1)->is_validation() && ((*it1)->_type != MASK) ) {
      continue;
    }
    
    it2 = it1;
    it2++;
    
    if ((it2 != this->_entry_list.end()) && (**it1 == **it2)) {
    
      // We allow loading these kinds of duplicates for 'within'
      bool still_duplicate = true;
      
      cDiffEntry& a = **it1;
      cDiffEntry& b = **it2;

      bool a_exists = a.entry_exists("within");
      bool b_exists = b.entry_exists("within");
      
      if (a_exists || b_exists) {
        
        if (!b_exists) still_duplicate = false;
        if (!a_exists) still_duplicate = false;
        
        string a_string = a.find("within")->second;
        string b_string = b.find("within")->second;
        
        vector<string> a_string_list = split(a_string, ":");
        vector<string> b_string_list = split(b_string, ":");
        
        size_t i = 0;
        while ( ( i < a_string_list.size() ) && (i < b_string_list.size()) ) {
          if (a_string_list[i] != b_string_list[i]) {
            still_duplicate = false;
          }
          i++;
        }
      }

      if (still_duplicate) {
        if (!file_parse_errors) {
          ERROR("Duplicate entries in Genome Diff:\n" + (*it1)->as_string() + "\n" + (*it2)->as_string()
              + "\nAdd a 'unique' tag to one if this is intentional.");
        } else {
          string line_number_1 = (**it1).entry_exists("_line_number") ? (**it1)["_line_number"] : "NA";
          uint32_t line_number_2 = (**it2).entry_exists("_line_number") ? from_string<uint32_t>((**it2)["_line_number"]) : 0;
          
          file_parse_errors->add_line_error(line_number_2, (*it2)->as_string(), "Attempt to add duplicate of this existing entry from line " + line_number_1 + ":\n" + substitute((*it1)->as_string(),"\t", "<tab>") + "\nAdd a 'unique' tag to one if this is intentional.", true);
        }
      }
    }
  }
  
}



//! Call to generate random mutations.
void cGenomeDiff::random_mutations(string exclusion_file,
                                   string type,
                                   uint32_t n_muts,
                                   uint32_t buffer,
                                   cReferenceSequences& ref_seq_info,
                                   bool verbose)
{
  //Parse input option into mutation type.
  //Also determine minimum size and maximum size if provided by user. 
  vector<string> type_options = split_on_any(type, ":-,");
  string mut_type = type_options[0];
  uint32_t min_size = 1, max_size = 1;
  uint32_t min_copy_number = 2, max_copy_number = 2;
  switch(type_options.size())
  {
    case 1:  {
    }  break;
      
    case 2:  {
      min_size = un(type_options[1]);
      max_size = un(type_options[1]);
    }  break;
      
    case 3:  {
      min_size = un(type_options[1]);
      max_size = un(type_options[2]);
    }  break;
    
    case 4:  {
      min_size = un(type_options[1]);
      max_size = un(type_options[2]);
      min_copy_number = un(type_options[3]);
      max_copy_number = un(type_options[3]);
    }  break;
    
    case 5:  {
      min_size = un(type_options[1]);
      max_size = un(type_options[2]);
      min_copy_number = un(type_options[3]);
      max_copy_number = un(type_options[4]);
    }  break;
      
    default:      
      ERROR("CANNOT PARSE: " + type);
  }

  // These are repeat regions that we want to avoid when placing mutations
  cFlaggedRegions repeat_match_regions;
  if (exclusion_file.size()) {
    repeat_match_regions.read_mummer(exclusion_file, ref_seq_info);
  }

  // Currently we can only use one reference sequence
  string ref_seq_id = ref_seq_info.seq_ids()[0];
  cAnnotatedSequence& ref = ref_seq_info[ref_seq_id];

  //Container for checking that future simulated mutations are a buffered distance apart.
  cFlaggedRegions used_mutation_regions;

  const uint32_t max_attempts = 1000;
  uint32_t n_attempts = max_attempts;

  srand(cSimFastqSequence::SEED_VALUE);
  buffer +=1;

  /* TYPICAL WORKFLOW:
   *    1) Chose a random mutation size.
   *
   *    2) Chose a random position.
   *        a) Create and normalize a mutation at that position.
   *        b) Check that the normalized position and size will not overlap an excluded region plus buffer.
   *        c) Check that the normalized position and size not be within buffer distance of 
   *          another mutation.
   *        d) If necessary, check for any mutation specific changes after normalization.
   *
   *    3) Repeat #2 until a valid position is found or the max number of attempts for that 
   *      mutation size is reached.
   *        a) If the max number of attempts is reached go back to #1.
   *        b) If valid, add mutation to GD and to used_mutation_regions.
   *
   *    4) Repeat #1 until N valid mutations are found or the max number of attempts for simulating
   *      N mutations is reached.
   *        a) If the max number of attempts is reached then no more mutations can be simulated.
   *
   */

  if (mut_type == "SNP" || mut_type == "INS" || mut_type == "DEL") {      
    
    while (n_muts && n_attempts) {

      cDiffEntry new_item;
      new_item._type = cDiffEntry::type_to_enum(mut_type);
      new_item["seq_id"] = ref.m_seq_id;

      uint32_t pos_1 = 0;
      uint32_t size = mut_type == "SNP" ? 0 : rand() % (max_size - min_size + 1) + min_size;
      uint32_t n_size_attempts = max_attempts;

      while (n_size_attempts) {
        pos_1 = (rand() % (ref.get_sequence_size() - buffer - size)) + buffer;
        new_item["position"] = to_string(pos_1);

        if (mut_type == "SNP") {
          new_item["new_seq"] = cSimFastqSequence::get_random_error_base(ref.get_sequence_1(pos_1));        
        } 
        else 
        if (mut_type == "INS") {
          string* new_seq = &new_item["new_seq"];
          new_seq->resize(size);
          generate(new_seq->begin(), new_seq->end(), cSimFastqSequence::get_random_insertion_base);
        }
        else
        if (mut_type == "DEL") {
          new_item["size"] = s(size);
        }

        new_item.normalize_to_sequence(ref, verbose);

        pos_1 = un(new_item["position"]);

        bool is_excluded = repeat_match_regions.is_flagged(ref.m_seq_id, pos_1 - buffer, pos_1 + size + buffer);
        bool is_near_mutation = used_mutation_regions.is_flagged(ref.m_seq_id, pos_1 - buffer, pos_1 + size + buffer);

        if (is_excluded || is_near_mutation) {
          --n_size_attempts;
        } else {
          break;
        }
      }

      if (n_size_attempts == 0) {
        --n_attempts;
      } else {
        --n_muts, n_attempts = max_attempts;
        this->add(new_item);
        used_mutation_regions.flag_region(ref.m_seq_id, pos_1, pos_1 + size);
      }

    }
  }
  else if (mut_type == "RMD") {
    //Get all available IS elements.
    cFeatureLocationList repeats = ref.m_repeat_locations;
    ASSERT(repeats.size(), "No repeat_regions / ISX elements in reference sequence.");
    CHECK(n_muts <= repeats.size(), "Too many deletions requested, creating a potential maximum of " + s(repeats.size()));


    //Unflag repeat-match regions which affect an IS element and then store them.
    cFlaggedRegions IS_element_regions;
    for (cFeatureLocationList::iterator it = repeats.begin(); it != repeats.end(); ++it) {
      cFeatureLocation& repeat = *it;
      cSequenceFeature* feature = repeat.get_feature();
      
      uint32_t start_1 = repeat.get_start_1();
      uint32_t end_1   = repeat.get_end_1();
      IS_element_regions.flag_region(ref.m_seq_id, start_1, end_1 + 1); //Want end_1 + 1 because a deletion will occur there.

      cFlaggedRegions::regions_t regions = repeat_match_regions.regions_that_overlap(ref.m_seq_id, start_1, end_1);

      if (regions.size()) {
        uint32_t lower = regions.begin()->first;
        uint32_t upper = regions.rbegin()->second;
        
        cerr << "\tRemoving repeat-match excluded region: " << lower << "-" << upper << endl;
        cerr << "\t\tFor IS element: " << (*feature)["name"] << "\t" << repeat.get_start_1() << "-" << repeat.get_end_1() << endl;
        cerr << endl;

        repeat_match_regions.remove(ref.m_seq_id, regions);

      }
    }

    while (n_muts && repeats.size() && n_attempts) {
      vector<cDiffEntry> valid_items;
      cFeatureLocationList::iterator it;

      //Randomly choose deletion size.
      uint32_t pos_1 = 0;
      uint32_t size = rand() % (max_size - min_size + 1) + min_size;
      uint32_t n_size_attempts = max_attempts;
      while (n_size_attempts) {
        //Randomly choose a repeat_region.
        it = repeats.begin();
        advance(it, rand() % repeats.size());

        //Collect potentially valid left_side and right_side positions.
        vector<int32_t> valid_pos_1;
        uint32_t start_1 = it->get_start_1(), end_1   = it->get_end_1();
        valid_pos_1.push_back(start_1 - size);
        valid_pos_1.push_back(end_1 + 1);

        //Evaluate that once normalized as DELs, the potentially valid mutations are not within an excluded region.
        for (vector<int32_t>::iterator jt = valid_pos_1.begin(); jt != valid_pos_1.end(); ++jt) {
          pos_1 = *jt ; 

          cDiffEntry temp_item;        
          temp_item._type = DEL;
          temp_item["seq_id"]   = ref.m_seq_id;
          temp_item["position"] = s(pos_1);        
          temp_item["size"]     = s(size);        

          bool overlaps_multiple_IS = IS_element_regions.regions_that_overlap(ref.m_seq_id, pos_1, pos_1 + size).size() > 1;
          bool is_excluded = repeat_match_regions.is_flagged(ref.m_seq_id, pos_1 - buffer, pos_1 + size + buffer);
          bool is_within_buffer = used_mutation_regions.is_flagged(ref.m_seq_id, pos_1 - buffer, pos_1 + size + buffer);

          if (!is_excluded && !is_within_buffer && !overlaps_multiple_IS) {
            valid_items.push_back(temp_item);
          }
        }

        if (valid_items.empty()) {
          --n_size_attempts;
        } else { 
          break;
        }
      }

      if (n_size_attempts == 0) {
        --n_attempts;
      } else {
        --n_muts, n_attempts = max_attempts;
        cDiffEntry new_item = valid_items[rand() % valid_items.size()];
        new_item["mediated"] = (*(it->get_feature()))["name"];
        this->add(new_item);
        pos_1 = un(new_item["position"]);
        size = un(new_item["size"]);
        used_mutation_regions.flag_region(ref.m_seq_id, pos_1, pos_1 + size);
        repeat_match_regions.flag_region(ref.m_seq_id, it->get_start_1(), it->get_end_1());

        if (verbose) {
          cerr << "[ISX]: " + (*(it->get_feature()))["name"] << "\t[start_1]: " << it->get_start_1() << "\t[end_1]: " << it->get_end_1() << endl;
          cerr << "\t[DEL]: " << new_item << endl;
          cerr << endl;
        }
        repeats.erase(it);

      }

    }
    //Validate mutations.

    //Check that neighboring mutations are a buffered distance from each other.
    diff_entry_list_t muts = this->mutation_list();
    diff_entry_list_t::iterator it = muts.begin();
    diff_entry_list_t::iterator jt = it;
    for (++jt; jt != muts.end(); ++it, ++jt) {
      //Left mutation.
      uint32_t lpos_1 = un((**it)["position"]);
      uint32_t lsize = un((**it)["size"]);

      //Right mutation.
      uint32_t rpos_1 = un((**jt)["position"]);

      ASSERT(static_cast<uint32_t>(abs(static_cast<int32_t>(lpos_1 + lsize - rpos_1))) >=  buffer,
          "Mutation: " + (*it)->to_spec().as_string() + "\n" +
          "\tand\n" +
          "Mutation: " + (*it)->to_spec().as_string() +"\n" +
          "are less then the buffered distance away from each other.");
    }

    //Check that each mutation only effects one IS element.
    for (it = muts.begin(); it != muts.end(); ++it) {
      uint32_t pos_1 = un((**it)["position"]);
      uint32_t size = un((**it)["size"]);
      uint32_t n_IS_elements = IS_element_regions.regions_that_overlap(ref.m_seq_id, pos_1, pos_1 + size).size();
      ASSERT(n_IS_elements == 1,
          "Mutation overlaps more then one IS element: " + (*it)->to_spec().as_string());
    }

    //Display IS elements that could not be used.
    for (cFeatureLocationList::iterator it = repeats.begin(); it != repeats.end(); ++it) {
      WARN("Unused IS element: " + (*(it->get_feature()))["name"]+ '\t' + s(it->get_start_1()) + '-' + s(it->get_end_1()));
    }

  }
  else
  if (mut_type == "AMP") {
    while (n_muts && n_attempts) {
      uint32_t pos_1 = 0;
      uint32_t size = rand() % (max_size - min_size + 1) + min_size;
      
      cDiffEntry new_item;        
      new_item._type = AMP;
      new_item["seq_id"] = ref.m_seq_id;

      uint32_t n_size_attempts = max_attempts;

      while (n_size_attempts) {
        pos_1 = (rand() % (ref.get_sequence_size() - buffer - size)) + buffer;

        new_item["position"] = to_string(pos_1);
        new_item["size"] = s(size);
        new_item["new_copy_number"] = s((rand() % (max_copy_number - min_copy_number + 1)) + min_copy_number);

        new_item.normalize_to_sequence(ref);
        new_item.erase("norm_pos");
        uint32_t norm_pos_1 = un(new_item["position"]);

        bool is_excluded      = repeat_match_regions.is_flagged(ref.m_seq_id, pos_1 - buffer, pos_1 + size + buffer);
        bool is_near_mutation = used_mutation_regions.is_flagged(ref.m_seq_id, pos_1 - buffer, pos_1 + size + buffer);
        bool is_new_pos_1     = pos_1 != norm_pos_1;
        bool is_new_type      = to_string(new_item._type) != mut_type;

        if (is_excluded  || is_near_mutation || is_new_pos_1 || is_new_type) {
          --n_size_attempts;
        } else {
          break;
        }

      }

      if (n_size_attempts == 0) {
        --n_attempts;
      } else {
        --n_muts, n_attempts = max_attempts;
        new_item.erase("norm");
        this->add(new_item);
        used_mutation_regions.flag_region(ref.m_seq_id, pos_1, pos_1 + (size * un(new_item["new_copy_number"])));
        if (verbose) {
          cerr << "\t" << new_item << endl;
        }

      }

    }

  }
  else 
  if (mut_type == "MOB") {
    //@JEB: Update.. it does. But this comment from Matt is too fun to remove.
    //ERROR("THE BRESEQENSTEIN MUTATION GENERATOR DOES NOT YET HANDLE MOBS\nESPECIALLY IF THEY HAVE TORCHES AND PITCHFORKS");

    cSequenceFeatureList repeats = ref.m_repeats;
    ASSERT(repeats.size(), "No repeat_regions / ISX elements in reference sequence.");

    while (n_muts && n_attempts) {
      uint32_t pos_1 = 0;
      uint32_t size = rand() % (max_size - min_size + 1) + min_size;
      int8_t strand = rand() % 2 ? 1 : -1;
      cSequenceFeatureList:: iterator it = repeats.begin();
      advance(it, rand() % repeats.size());

      uint32_t n_size_attempts = max_attempts;
      while (n_size_attempts) {
        pos_1 = (rand() % (ref.get_sequence_size() - buffer)) + buffer;
        string temp_seq = ref.get_sequence_1(pos_1, pos_1 + size - 1);
        cDiffEntry temp_item;        
        temp_item._type = INS;
        temp_item["seq_id"] = ref.m_seq_id;
        temp_item["position"] = s(pos_1);
        temp_item["new_seq"] = temp_seq;
        
        temp_item.normalize_to_sequence(ref);
        uint32_t norm_pos_1   = un(temp_item["position"]);

        bool is_excluded      = repeat_match_regions.is_flagged(ref.m_seq_id, norm_pos_1 - buffer, norm_pos_1 + size + buffer);
        bool is_near_mutation = used_mutation_regions.is_flagged(ref.m_seq_id, norm_pos_1 - buffer, norm_pos_1 + size + buffer);
        bool is_new_pos_1     = pos_1 != norm_pos_1; 
        bool is_new_seq       = temp_seq != temp_item["new_seq"];
        bool is_not_INS       = temp_item._type != INS;

        if (is_excluded || is_near_mutation || is_new_pos_1 || is_new_seq || is_not_INS) {
          --n_size_attempts;
        } else {
          break;
        }

      }

      if (n_size_attempts == 0) {
        --n_attempts;
      } else {
        --n_muts, n_attempts = max_attempts;
        cDiffEntry new_item;        
        new_item._type = MOB;
        new_item["seq_id"] = ref.m_seq_id;
        new_item["position"] = s(pos_1);
        new_item["repeat_name"] = (*it)->SafeGet("name");
        new_item["strand"] = s(strand);        
        new_item["duplication_size"] = s(size);

        new_item.normalize_to_sequence(ref);
        new_item.erase("norm_pos");

        this->add(new_item);
        used_mutation_regions.flag_region(ref.m_seq_id, pos_1, pos_1 + size);

        if (verbose) {
          cerr << "\t" << new_item << endl;
        }

      }

    }
  }
  else { 
    ERROR("MUTATION TYPE NOT HANDLED: " + mut_type);
  }
  
  CHECK(max_attempts == n_attempts, "Forced to halt mutation generation.\nAttempted " +
        s(max_attempts - n_attempts) + " times to generate another mutation.\n" + 
        "It's likely that it's no longer possible to add new mutations.");
  
}
  
// Shifts all entries in file as appropriate for applying current_mut
  
void cGenomeDiff::shift_positions(cDiffEntry &current_mut, cReferenceSequences& ref_seq_info, bool verbose)
{  
  int32_t delta = current_mut.mutation_size_change(ref_seq_info);
  
  if (verbose) {
    if (current_mut._type != INV) {
      cout << "Shifting remaining entries by: " << delta << " bp." << endl;
    } else {
      cout << "Inverting region: " << current_mut[POSITION] << "-" << s(n(current_mut[POSITION]) + n(current_mut[SIZE]) - 1) << "." << endl;
    }
  }
  if (delta == UNDEFINED_INT32)
    ERROR("Size change not defined for mutation.");
  
  cReferenceCoordinate shift_start = current_mut.get_reference_coordinate_start();
  cReferenceCoordinate shift_end = current_mut.get_reference_coordinate_start();

  diff_entry_list_t muts = this->mutation_list();
  for (diff_entry_list_t::iterator itr = muts.begin(); itr != muts.end(); itr++) {
    cDiffEntry& mut = **itr;
  
    // Mutations don't shift themselves
    if (mut == current_mut) continue;
    
    // the position to be updated
    int32_t position = from_string<uint32_t>(mut[POSITION]);

    // Check to see whether this is listed as being nested within the current item
    
    // Special case -- we are nested within current_mut so coord updates are more complicated
    bool was_nested = false;
    
    if (mut.entry_exists("within")) {
      
      //  Form is mutation_id:copy_index
      //  For MOB/INS, index can be blank, which implies the mutation is within the new sequence
      //  Note that the following code is not guaranteed safe unless validate_with_reference_sequence has been called
 
      vector<string> split_within = split(mut["within"], ":");
      string within_mutation_id = split_within[0];
      
      // Mutation that we are CURRENTLY SHIFTING is within the mutation that we are shifting FOR
      if (current_mut._id == within_mutation_id) {
        was_nested = true; // handle offset here
        int32_t within_copy_index = -1;     

        if (split_within.size() == 2) {
          within_copy_index = from_string<int32_t>(split_within[1]);
        }

        uint64_t special_delta(0);

        if (current_mut._type == AMP) {
          
          // Inside an AMP means we shift to the desired copy
          uint64_t amp_unit_size = from_string<uint64_t>(current_mut[SIZE]);
          
          // Special case of IS mediated AMP, must add in size of IS element to offset
          if (current_mut.entry_exists(MEDIATED)) {
            
            cFeatureLocation repeat_feature_picked;
            string seq_id_picked;
            string mediated_string;
            string mob_region;
            
            if (current_mut.entry_exists(MOB_REGION)) {
              mob_region = current_mut[MOB_REGION];
            }
            
            mediated_string = ref_seq_info.repeat_family_sequence(current_mut[MEDIATED], from_string<int16_t>(current_mut[MEDIATED_STRAND]), mob_region.size() ? &mob_region : NULL, &seq_id_picked, &repeat_feature_picked);
            
            amp_unit_size += mediated_string.size();
          }
          
          special_delta = amp_unit_size * (within_copy_index-1);
          
        } else if (current_mut._type == MOB) {
          // Normal delta is size of mobile element plus the duplication size
          // this is the default if we are in the second copy
          if (within_copy_index == 2) {
            special_delta = delta;
          } else {
            // For first copy (within_copy_index=1) or inside the IS (within_copy_index -1) we don't need to move things
            special_delta = 0;
          }
         
        } else if ( current_mut._type == INS ) {
          // Inside an INS means copy_index is actually the nucleotide within the insertion
          special_delta = within_copy_index;
        }
        
        // -1 used as offset to force update of position because we are within it...
        mut.mutation_shift_position(current_mut[SEQ_ID], cReferenceCoordinate(-1, 0), cReferenceCoordinate(-1, 0), special_delta);
      }

      // Mutation is within a mutation that is within the current mutation... do not update size
      else {
        diff_entry_ptr_t within_mutation = find_by_id(within_mutation_id);
        
        if (within_mutation.get() != NULL) {
          if (within_mutation->entry_exists("within")) {
            vector<string> split_within_within = split((*within_mutation)["within"],":");
            string within_within_mutation_id = split_within_within[0];
            diff_entry_ptr_t within_within_mutation = find_by_id(within_within_mutation_id);

            if (within_within_mutation.get() != NULL) {
              ASSERT(!within_within_mutation->entry_exists("within"), "Too many nested withing mutations, starting with:\n" + mut.as_string());
            }
            
            if (current_mut._id == within_within_mutation_id) {
              was_nested = true; // handle offset here
              
              // Same as a normal mutation but we don't change the size
              mut.mutation_shift_position(current_mut[SEQ_ID], cReferenceCoordinate(-1, 0), cReferenceCoordinate(-1, 0), delta);
            }
          }
        }
      }
      
    }
    
    // Normal behavior -- offset mutations later in same reference sequence
    if (!was_nested) {
      if (current_mut._type == INV) {
          mut.mutation_invert_position_sequence(current_mut);
      } else {
        mut.mutation_shift_position(current_mut[SEQ_ID], shift_start, shift_end, delta);
      }
    }
  }
}

// The return sequence includes just the MOB part to be inserted
// It DOES include any deleted/inserted bases at the end of the MOB.
// It DOES NOT include any target site duplication. 
string cGenomeDiff::mob_replace_sequence(cReferenceSequences& ref_seq_info, 
                                         cDiffEntry& mut, 
                                         string* picked_seq_id, 
                                         cFeatureLocation* picked_sequence_feature)
{
  bool verbose = false;

  ASSERT(mut._type == MOB, "Attempt to get mob_replace_sequence for non-MOB diff entry.");

  if (verbose) cout << "Building MOB replace sequence..." << endl << mut << endl;
    
  //Size to delete from start of repeat string.
  int32_t iDelStart = 0;
  int32_t iDelEnd = 0;
  if(mut.entry_exists("del_start")) 
    iDelStart = from_string<int32_t>(mut["del_start"]);
  if(mut.entry_exists("del_end"))   
    iDelEnd = from_string<int32_t>(mut["del_end"]);
  ASSERT((iDelStart >= 0) && (iDelEnd >= 0), (to_string(mut._type) + " " + mut._id) + " - NEGATIVE DELETION");
    
  // @JEB: We look for repeat sequence in original reference sequence.
  //       This saves us from possibly looking at a shifted location...
  string rep_string = ref_seq_info.repeat_family_sequence(mut["repeat_name"], from_string<int16_t>(mut["strand"]), mut.entry_exists("mob_region") ? &mut["mob_region"] : NULL, picked_seq_id, picked_sequence_feature);
  mut["repeat_size"] = to_string(rep_string.length()); // saving this for shifting
  
  // This is the string we're going to pass to be inserted.
  // It will eventually contain the repeat string, insertions
  // and the duplicate_sequence.
  string new_seq_string = rep_string;
  
  // Do we have deletes?  Go ahead and delete them from the repeat.
  // This happens before inserts -- deletes are always part of the repeat element.
  if(iDelStart)
    new_seq_string.replace(0,iDelStart,"");
  if(iDelEnd)
    new_seq_string.resize(new_seq_string.size() - iDelEnd);
  
  // If there are any inserts, put them in front of or behind the repeat sequence.
  if(mut.entry_exists("ins_start")) {
    new_seq_string = mut["ins_start"] + new_seq_string;
  }
  
  if(mut.entry_exists("ins_end"))   {
    new_seq_string += mut["ins_end"];
  }
  
  if (verbose) cout << "  Final sequence:" << endl << new_seq_string << endl;
  
  return new_seq_string;
}
  
// This function will use the current GD and apply it to the new_ref_seq_info.
// When calling this function make SURE that you load ref_seq_info and
// new_ref_seq_info separately.
//
// This function also annotates IS-mediates, adjacent, etc. mutations
//
// Finally, it will set bases added, bases changed, bases deleted in the GD header
//
void cGenomeDiff::apply_to_sequences(cReferenceSequences& ref_seq_info, cReferenceSequences& new_ref_seq_info, bool verbose, int32_t slop_distance, int32_t size_cutoff_AMP_becomes_INS_DEL_mutation)
{    
  uint32_t count_SNP = 0, count_SUB = 0, count_INS = 0, count_DEL = 0, count_AMP = 0, count_INV = 0, count_MOB = 0, count_CON = 0, count_MASK = 0;
  uint32_t bases_inserted(0), bases_deleted(0), bases_changed(0);
  
  // Sort the list into apply order ('within' and 'before' tags)
  cGenomeDiff::sort_apply_order();
  
  // Handle all mutation types, plus MASK four-letter type.
  diff_entry_list_t mutation_list = this->mutation_list();
  diff_entry_list_t mask_list = this->get_list(make_vector<gd_entry_type>(MASK));
  mutation_list.insert(mutation_list.end(), mask_list.begin(), mask_list.end());
  
  for (diff_entry_list_t::iterator itr_mut = mutation_list.begin(); itr_mut != mutation_list.end(); itr_mut++)
  {
    cDiffEntry& mut(**itr_mut);
    uint32_t position = from_string<uint32_t>(mut[POSITION]);
    
    // Look out! -- you should not apply things that don't have frequency=1 or other markers of polymorphism mode
    //ASSERT(!((mut._type == INS) && (mut.count(INSERT_POSITION))), "Attempt to apply insertion with \"insert_position\" field set, which indicates your Genome Diff represents polymorphic mutations.\n" + mut.as_string());
    if ((mut.count(FREQUENCY)!=0) && (from_string<double>(mut[FREQUENCY]) != 1.0)) {
      WARN("Attempt to apply polymorphic mutation with frequency != 1. This mutation will be skipped.\n" + mut.as_string());
      continue;
    }
    
    // Mark 'between', 'mediated', 'adjacent' to repeat_region mutations
    //   Must be done before we apply the current mutation to the sequence
    //   But previous mutations must be applied (because for example it may be mediated by a *new* IS copy).
    mut.annotate_repeat_hotspots(new_ref_seq_info, slop_distance, size_cutoff_AMP_becomes_INS_DEL_mutation, false);
    
    // Attributes used for output of debug info
    string replace_seq_id;
    uint32_t replace_start;
    uint32_t replace_end;
    string applied_seq_id;
    uint32_t applied_start;
    uint32_t applied_end;
    string replace_seq;
    string applied_seq;
    
    if (verbose) cout << endl << "APPLYING MUTATION:" << endl << mut << endl;
    
    switch (mut._type) 
    {
      case SNP :
      {
        count_SNP++;
        
        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        replace_start = position;
        replace_end = position;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        
        applied_seq_id = mut[SEQ_ID];
        applied_start = position;
        applied_end = position;
        applied_seq = mut[NEW_SEQ];
        
        // Replace sequence
        new_ref_seq_info.replace_sequence_1(mut[SEQ_ID], position, position, mut[NEW_SEQ], (to_string(mut._type) + " " + mut._id));
        
        bases_changed++;
        
      } break;
        
      case SUB:
      {
        count_SUB++;
        
        int32_t size = from_string<int32_t>(mut[SIZE]);
        if (mut.entry_exists(APPLY_SIZE_ADJUST)) {
          size += from_string<int32_t>(mut[APPLY_SIZE_ADJUST]);
          ASSERT(size > 0, "Attempt to apply mutation with non-positive size.");
        }

        // Set up attributes (includes base before for cases of no replaced bases)
        replace_seq_id = mut[SEQ_ID];
        replace_start = position - 1;
        replace_end = position - 1 + size;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        replace_seq.insert(0,"(");
        replace_seq.insert(2,")");
        
        applied_seq_id = mut[SEQ_ID];
        applied_start = position - 1;
        applied_end = position - 1 + mut[NEW_SEQ].size();
        applied_seq = replace_seq.substr(0,3) + mut[NEW_SEQ];
        
        new_ref_seq_info.replace_sequence_1(mut[SEQ_ID], position, position + size - 1, mut[NEW_SEQ], (to_string(mut._type) + " " + mut._id));
        
        bases_changed  += min(size, static_cast<int32_t>(mut[NEW_SEQ].size()));
        bases_deleted  += max(0, size - static_cast<int32_t>(mut[NEW_SEQ].size()));
        bases_inserted += max(0, static_cast<int32_t>(mut[NEW_SEQ].size()) - size);
        
      } break;
        
      case INS:
      {          
        count_INS++;
        
        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        replace_start = position;
        replace_end = position;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        replace_seq.insert(0,"(");
        replace_seq.insert(replace_seq.size(),")");
        
        applied_seq_id = mut[SEQ_ID];
        applied_start = position;
        applied_end = position + mut[NEW_SEQ].size();
        applied_seq = replace_seq + mut[NEW_SEQ];
        
        new_ref_seq_info.insert_sequence_1(mut[SEQ_ID], position, mut[NEW_SEQ], (to_string(mut._type) + " " + mut._id));
        
        bases_inserted += mut[NEW_SEQ].size();
        
      } break;
        
      case DEL:
      {
        count_DEL++;
        
        int32_t size = from_string<int32_t>(mut[SIZE]);
        if (mut.entry_exists(APPLY_SIZE_ADJUST)) {
          size += from_string<int32_t>(mut[APPLY_SIZE_ADJUST]);
          ASSERT(size > 0, "Attempt to apply mutation with non-positive size.");
        }
        
        // We normally show the base before (if there is one)
        int32_t remaining_base_start_shown = 1;
        int32_t remaining_base_end_shown = 0;

        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        
        if (position == 1) {
          // Show the base after if deletion starts on first base
          remaining_base_start_shown = 0;
          remaining_base_end_shown = 1;
          
          // Show no bases if deletion encompasses entire genome fragment
          if ( position - 1 + size == new_ref_seq_info[replace_seq_id].get_sequence_length() ) {
            remaining_base_end_shown = 0;
          }
        }
        
        replace_start = position - remaining_base_start_shown;
        replace_end = position - 1 + size + remaining_base_end_shown;
        
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        if (remaining_base_start_shown) {
          replace_seq.insert(0,"(");
          replace_seq.insert(2,")");
        } else if (remaining_base_end_shown) {
          replace_seq.insert(replace_seq.size()-1,"(");
          replace_seq.insert(replace_seq.size(),")");
        } else {
          replace_seq.insert(0, "(.)");
        }
        
        applied_seq_id = mut[SEQ_ID];
        if (remaining_base_start_shown) {
          applied_start = position - 1;
          applied_end = position - 1;
          applied_seq = new_ref_seq_info.get_sequence_1(applied_seq_id, applied_start, applied_end);
        } else if (remaining_base_end_shown) {
          applied_start = position - 1 + size + remaining_base_end_shown;
          applied_end = position - 1 + size + remaining_base_end_shown;
          applied_seq = new_ref_seq_info.get_sequence_1(applied_seq_id, applied_start, applied_end);
        } else {
          applied_start = 0;
          applied_end = 0;
          applied_seq = ".";
        }
        applied_seq.insert(0,"(");
        applied_seq.insert(2,")");

        ASSERT(size >= 1, "Attempt to apply DEL mutation with size ≤ 0:\n" + mut.as_string());
        new_ref_seq_info.replace_sequence_1(mut[SEQ_ID], position, position + size -1, "", (to_string(mut._type) + " " + mut._id));
        
        bases_deleted += size;
        
      } break;
        
      case MASK:
      {
        count_MASK++;
        const uint32_t& size = from_string<uint32_t>(mut[SIZE]);
        string mask_string(size, 'N');
        
        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        replace_start = position;
        replace_end = position + size - 1;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        
        applied_seq_id = mut[SEQ_ID];
        applied_start = position;
        applied_end = position + size - 1;
        applied_seq = mask_string;
        
        new_ref_seq_info.replace_sequence_1(mut[SEQ_ID], position, position + size - 1, mask_string, (to_string(mut._type) + " " + mut._id));
        
      } break;
        
      case AMP:
      {
        count_AMP++;
        
        int32_t size = from_string<int32_t>(mut[SIZE]);
        if (mut.entry_exists(APPLY_SIZE_ADJUST)) {
          size += from_string<int32_t>(mut[APPLY_SIZE_ADJUST]);
          ASSERT(size > 0, "Attempt to apply mutation with non-positive size.");
        }
        
        // Special case of mediated AMP
        cFeatureLocation repeat_feature_picked;
        string seq_id_picked;
        string mediated_string;
        if (mut.entry_exists(MEDIATED)) {
          //cout << mut.as_string() << endl;
          mediated_string = ref_seq_info.repeat_family_sequence(mut[MEDIATED], from_string<int16_t>(mut[MEDIATED_STRAND]), mut.entry_exists("mob_region") ? &mut["mob_region"] : NULL, &seq_id_picked, &repeat_feature_picked);
        }

        // We insert the pieces one at a time, so that we can update annotations
        cLocation duplicated_region;
        cLocation duplicated_region_wrap;

        bool wraps_around_circular_chromosome = (position+size-1 > new_ref_seq_info.get_sequence_length(mut[SEQ_ID]));
        
        string duplicated_sequence_one_copy;
        if (!wraps_around_circular_chromosome) {
          duplicated_region = cLocation(position, position+size-1, 1);
          duplicated_sequence_one_copy = new_ref_seq_info.get_sequence_1(mut[SEQ_ID], position, position+size-1);
        } else {
          duplicated_sequence_one_copy = new_ref_seq_info.get_sequence_1(mut[SEQ_ID], position, new_ref_seq_info.get_sequence_length(mut[SEQ_ID]));
          duplicated_sequence_one_copy += new_ref_seq_info.get_sequence_1(mut[SEQ_ID], 1, position+size-1 - new_ref_seq_info.get_sequence_length(mut[SEQ_ID]));
          duplicated_region = cLocation(position, new_ref_seq_info.get_sequence_length(mut[SEQ_ID]), 1);
          duplicated_region_wrap = cLocation(1, position+size-1 - new_ref_seq_info.get_sequence_length(mut[SEQ_ID]), 1);
        }
        
        //Also build full duplicated sequence (strictly for debugging output)
        string duplicated_sequence_full_addition;

        // Loop executed new_copy_number - 1 times.
        for (uint32_t i = 1; i < from_string<uint32_t>(mut["new_copy_number"]); i++) {
          
          // Everything is inserted BEFORE the existing copy, pushing the earlier copies forward!
          
          // <---------------- Order of insertion
          // [++++ => ++++ =>] ++++
          
          // We have to insert a copy of the IS element first (so all copies end up between repeats)
          if (mediated_string.size()) {
            
            // inserted AFTER specified position
            new_ref_seq_info.insert_sequence_1(mut[SEQ_ID], position-1, mediated_string, (to_string(mut._type) + " " + mut._id));
            
            // We've repeated the sequence, now it's time to repeat all the features
            // inside of and including the repeat region.
            
            // Starts at specified position (NOT after it)
            new_ref_seq_info.repeat_feature_1(mut[SEQ_ID], position, 0, 0, ref_seq_info, seq_id_picked, from_string<int16_t>(mut[MEDIATED_STRAND]), repeat_feature_picked);
            
            // This is constructed *only* for debugging output!
            duplicated_sequence_full_addition = "[" + mediated_string + "]" + duplicated_sequence_full_addition;
          }
          
          
          // inserted AFTER specified position
          new_ref_seq_info.insert_sequence_1(mut[SEQ_ID], position-1, duplicated_sequence_one_copy, (to_string(mut._type) + " " + mut._id));
          
          // Starts at specified position (NOT after it)
          new_ref_seq_info.repeat_feature_1(mut[SEQ_ID], position, 0, 0, ref_seq_info, mut[SEQ_ID], +1, duplicated_region);

          // TEMPORARY check
          new_ref_seq_info.update_feature_lists();
          
          if (wraps_around_circular_chromosome) {
            new_ref_seq_info.repeat_feature_1(mut[SEQ_ID], position + (new_ref_seq_info.get_sequence_length(mut[SEQ_ID]) - position) + 1, 0, 0, ref_seq_info, mut[SEQ_ID], +1, duplicated_region_wrap);
          }
          
          // This is constructed *only* for debugging output!
          duplicated_sequence_full_addition = duplicated_sequence_one_copy + duplicated_sequence_full_addition;
        }
        ASSERT(!duplicated_sequence_full_addition.empty(), "Duplicate sequence is empty. You may have specified an AMP with a new copy number of 1.");
        
        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        replace_start = position;
        replace_end = position + size - 1;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        
        applied_seq_id = mut[SEQ_ID];
        applied_start = position;
        applied_end = position + duplicated_sequence_full_addition.size() - 1;
        applied_seq = duplicated_sequence_full_addition + replace_seq;
        
        bases_inserted += duplicated_sequence_full_addition.size();
        
      } break;
        
      case INV:
      {
        int32_t size = from_string<int32_t>(mut[SIZE]);
        if (mut.entry_exists(APPLY_SIZE_ADJUST)) {
          size += from_string<int32_t>(mut[APPLY_SIZE_ADJUST]);
          ASSERT(size > 0, "Attempt to apply mutation with non-positive size.");
        }

        replace_seq_id = mut[SEQ_ID];
        replace_start = position;
        replace_end = position + size - 1;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        
        string inv_seq = reverse_complement(replace_seq);

        applied_seq_id = mut[SEQ_ID];
        applied_start = position + inv_seq.size() - 1;
        applied_end = position;
        applied_seq = inv_seq;
        
        count_INV++;
        new_ref_seq_info.invert_sequence_1(mut[SEQ_ID], position, position + size - 1, (to_string(mut._type) + " " + mut._id));
        
      } break;
        
      case MOB:
      {
        ASSERT(mut["strand"] != "?", "Unknown MOB strand (?)\n" + mut.as_string());
        
        count_MOB++;
        int32_t iDelStart = 0;
        int32_t iDelEnd = 0;
        int32_t iInsStart = 0;
        int32_t iInsEnd = 0;        
        int32_t iDupLen = 0;
        int32_t iDupSeqLen = 0; // Size of any sequence inserted
        
        //Size to delete from start of repeat string.
        if(mut.entry_exists("del_start")) 
          iDelStart = from_string<int32_t>(mut["del_start"]);
        if(mut.entry_exists("del_end"))   
          iDelEnd = from_string<int32_t>(mut["del_end"]);
        ASSERT((iDelStart >= 0) && (iDelEnd >= 0), (to_string(mut._type) + " " + mut._id) + " - NEGATIVE DELETION");
             
        if(mut.entry_exists("duplication_size"))
          iDupLen = from_string<int32_t>(mut["duplication_size"]);
          
        
        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        replace_start = position - 1;
        replace_end = position - 1 + abs(iDupLen);
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        replace_seq.insert(0,"(");
        replace_seq.insert(2,")");
        if (iDupLen > 0) {
          replace_seq.insert(3, "[");
          replace_seq.insert(4 + iDupLen, "]");
        }
        else if (iDupLen < 0) {
          replace_seq.insert(3, "<");
          replace_seq.insert(4 + abs(iDupLen), ">");
        }
          
        applied_seq_id = mut[SEQ_ID];
        applied_start = position - 1;
        applied_end = position - 1;
        applied_seq = new_ref_seq_info.get_sequence_1(applied_seq_id, applied_start, applied_end);
        applied_seq.insert(0,"(");
        applied_seq.insert(2,")");

        if (iDupLen > 0) {
          iDupSeqLen = iDupLen;
        } else if (iDupLen < 0) {
          // A negative duplication length indicates that this many bases were deleted from the 
          // original genome starting at the specified base. Note that is does not affect the later insert
          // which occurs prior to this location.
          new_ref_seq_info.replace_sequence_1(mut[SEQ_ID], position, position + abs(iDupLen)-1, "");
        }
        
        // If there are any inserts, put them in front of or behind the
        // repeat sequence.
        if(mut.entry_exists("ins_start")) 
          iInsStart = mut["ins_start"].length();
        if(mut.entry_exists("ins_end"))
          iInsEnd = mut["ins_end"].length();
        
        
        // Duplicated region must be from new ref seq, b/c the position
        // of the mutation has been shifted at this point.
        string new_seq_string;
        if (iDupLen > 0) new_seq_string = new_ref_seq_info.get_sequence_1(replace_seq_id, position, position + iDupLen - 1);
        
        // This includes all but the duplicated target site bases --
        // notice we pass the original reference sequence in case
        // a relevant feature has been deleted in the new reference
        cFeatureLocation repeat_feature_picked;
        string seq_id_picked;
        new_seq_string += mob_replace_sequence(ref_seq_info, mut, &seq_id_picked, &repeat_feature_picked);

        //if (verbose) cout << new_seq_string << endl;
        
        applied_end = position - 1 + new_seq_string.size();
        applied_seq += new_seq_string;
        if (iDupLen > 0) {
          applied_seq.insert(3, "[");
          applied_seq.insert(4 + iDupLen, "]");
          applied_seq += "[" + new_ref_seq_info.get_sequence_1(replace_seq_id, position, position + iDupLen - 1) + "]";
        }
        
        // @JEB 2014-01-05
        // Special case for target site duplication of size zero so that
        // we always insert AFTER the specified position. Without this correction
        // we insert before the specified base when target site duplication is zero.
        if (iDupLen == 0) {
          position = position + 1;
        }
        
        // The position of a MOB is the first position that is duplicated
        // Inserting at the position means we have to copy the duplication
        // in FRONT OF the repeat sequence
        new_ref_seq_info.insert_sequence_1(mut[SEQ_ID], position-1, new_seq_string, (to_string(mut._type) + " " + mut._id));
        
        // We've repeated the sequence, now it's time to repeat all the features
        // inside of and including the repeat region.
        new_ref_seq_info.repeat_feature_1(mut[SEQ_ID], position+iInsStart+iDupSeqLen, iDelStart, iDelEnd, ref_seq_info, seq_id_picked, from_string<int16_t>(mut["strand"]), repeat_feature_picked);
        
        bases_inserted += new_seq_string.size();
        
      } break;
        
      case CON:
      {
        count_CON++;
        
        int32_t size = from_string<int32_t>(mut[SIZE]);
        if (mut.entry_exists(APPLY_SIZE_ADJUST)) {
          size += from_string<int32_t>(mut[APPLY_SIZE_ADJUST]);
          ASSERT(size > 0, "Attempt to apply mutation with non-positive size.");
        }
        
        uint32_t replace_target_id, replace_start, replace_end;
        new_ref_seq_info.parse_region(mut["region"], replace_target_id, replace_start, replace_end);
        ASSERT(replace_start != replace_end, "Cannot process CON mutation with end == start. ID:" + mut._id);
        
        int8_t strand = (replace_start < replace_end) ?  +1 : -1;
        
        if (strand == -1) {
          swap(replace_start, replace_end);
        }
        
        // @JEB: correct here to look for where the replacing_sequence is in the original ref_seq_info.
        // This saves us from possible looking at a shifted location...
        string replacing_sequence = ref_seq_info[replace_target_id].get_sequence_1(replace_start, replace_end);
        
        if (strand == -1) {
          replacing_sequence = reverse_complement(replacing_sequence);
        }
        
        string displaced_sequence = new_ref_seq_info.get_sequence_1(mut[SEQ_ID], position, position + size - 1);
        
        // Set up attributes
        replace_seq_id = mut[SEQ_ID];
        replace_start = position - 1;
        replace_end = position - 1;
        replace_seq = new_ref_seq_info.get_sequence_1(replace_seq_id, replace_start, replace_end);
        replace_seq.insert(0,"(");
        replace_seq.insert(2,")");
        
        applied_seq_id = mut[SEQ_ID];
        applied_start = position - 1;
        applied_end = position - 1 + replacing_sequence.size();
        applied_seq = replace_seq + replacing_sequence;
        
        new_ref_seq_info.replace_sequence_1(mut[SEQ_ID], position, position + size - 1, replacing_sequence, (to_string(mut._type) + " " + mut._id)); 
              
      } break;
        
      default:
        ASSERT(false, "Can't handle mutation type: " + to_string(mut._type));
    }
    
    if (verbose)
    {
      cout << "Replacing: " << replace_seq_id << ":" << replace_start << "-" << replace_end << endl;
      cout << "(Sequence) " << replace_seq << endl;
      cout << "With:      " << applied_seq_id << ":" << applied_start << "-" << applied_end << endl;
      cout << "(Sequence) " << applied_seq << endl;
    }
    
    this->shift_positions(mut, new_ref_seq_info, verbose);
    
    // TEMPORARY! Check earlier mutations
    int32_t i=0;
    diff_entry_list_t::iterator prev_it = itr_mut;
    if (prev_it != mutation_list.begin()) prev_it--;
    while ( (prev_it != mutation_list.begin()) && (i < 5) ) {
      (*prev_it)->annotate_repeat_hotspots(new_ref_seq_info, slop_distance, size_cutoff_AMP_becomes_INS_DEL_mutation, false, true);
      prev_it--;
      i++;
    }
    
    // Re-sort ... invalidates the iterator
    if (mut._type == INV) {
      // Sort the list into apply order ('within' and 'before' tags)
      cGenomeDiff::sort_apply_order();
    }
    
  }
  
  // BEGIN Fix duplicate accessions locus_tags
  map<string,int32_t> locus_tag_tracker;
  for (vector<cAnnotatedSequence>::iterator it_as = new_ref_seq_info.begin(); it_as < new_ref_seq_info.end(); it_as++) {
    for(cSequenceFeatureList::iterator it_feat = it_as->m_features.begin(); it_feat != it_as->m_features.end(); it_feat++) {
      if ((**it_feat)["accession"] == "") continue;
      string type_accession =  (**it_feat)["type"] + "__" + (**it_feat)["accession"];
      if (locus_tag_tracker.find(type_accession) == locus_tag_tracker.end())
        locus_tag_tracker[type_accession]=1;
      else
        locus_tag_tracker[type_accession]++;
    }
  }
  
  // Second pass, rename all that occur multiple times
  map<string,int32_t> locus_tag_tracker_count;
  for (vector<cAnnotatedSequence>::iterator it_as = new_ref_seq_info.begin(); it_as < new_ref_seq_info.end(); it_as++) {
    for(cSequenceFeatureList::iterator it_feat = it_as->m_features.begin(); it_feat != it_as->m_features.end(); it_feat++) {
      if ((**it_feat)["accession"] == "") continue;
      string type_accession =  (**it_feat)["type"] +  "__" + (**it_feat)["accession"];
      
      if (locus_tag_tracker[type_accession] == 1) continue;
      
      if (locus_tag_tracker_count.find(type_accession) == locus_tag_tracker_count.end())
        locus_tag_tracker_count[type_accession]=1;
      else
        locus_tag_tracker_count[type_accession]++;
      
       (*it_feat)->append_to_accession("_" + to_string<int32_t>(locus_tag_tracker_count[type_accession]) );
    }
  }
  // END  fix of duplicate accessions
  
  
  // Fix all of the sublists we have not been paying attention to
  new_ref_seq_info.update_feature_lists();
  
  // Regenerate
  
  if (verbose)
  {    
    cout << "MUTATION COUNT" << endl;
    cout << "\tSNP: " << count_SNP << endl;
    cout << "\tSUB: " << count_SUB << endl;
    cout << "\tINS: " << count_INS << endl;
    cout << "\tDEL: " << count_DEL << endl;
    cout << "\tAMP: " << count_AMP << endl;
    cout << "\tINV: " << count_INV << endl;
    cout << "\tMOB: " << count_MOB << endl;
    cout << "\tCON: " << count_CON << endl;
    cout << "\tMASK: " << count_MASK << endl;
  }
  
  this->add_breseq_data("BASES-CHANGED", to_string<uint32_t>(bases_changed));
  this->add_breseq_data("BASES-INSERTED", to_string<uint32_t>(bases_inserted));
  this->add_breseq_data("BASES-DELETED", to_string<uint32_t>(bases_deleted));
  this->add_breseq_data("GENOME-SIZE-INITIAL", to_string(ref_seq_info.total_length()));
  this->add_breseq_data("GENOME-SIZE-FINAL", to_string(new_ref_seq_info.total_length()));
  
  //Cleanup.  If any of the sequences are of zero length, remove them.
  for (vector<cAnnotatedSequence>::iterator it_as = new_ref_seq_info.begin(); it_as < new_ref_seq_info.end(); it_as++) {
    if(!it_as->m_length){new_ref_seq_info.erase(it_as);it_as--;}
  }
  
}
  
void cGenomeDiff::mask_mutations(cGenomeDiff& mask_gd, bool mask_only_small, bool verbose)
{
  const int32_t mask_small_max_size_limit = 20;
  
  diff_entry_list_t masks = mask_gd.get_list(make_vector<gd_entry_type>(MASK));
  
  // Create all of the flagged regions
  cFlaggedRegions flagged_regions;
  for (diff_entry_list_t::iterator mask_it = masks.begin(); mask_it != masks.end(); mask_it++) {
    diff_entry_ptr_t& mask = *mask_it;
    flagged_regions.flag_region(mask->at(SEQ_ID), from_string<uint32_t>(mask->at(POSITION)), from_string<uint32_t>(mask->at(POSITION)) + from_string<uint32_t>(mask->at(SIZE)) - 1);
  }
  
  // Mask mutations by removing entries from the current GD file
  diff_entry_list_t::iterator mut_it = this->_entry_list.begin();
  
  bool advance_it(true);
  while (mut_it != this->_entry_list.end()) {
    
    diff_entry_ptr_t& mut = *mut_it;
    
    // ONLY mutations
    if (!mut->is_mutation()) {
      ++mut_it;
      continue;
    }
    
    // Bail if we are not a small mutation
    bool is_small = false;
    if (mask_only_small) {
      
      is_small = mut->is_small_mutation(mask_small_max_size_limit);
      
      // Don't remove these...
      if ( mut->entry_exists(MEDIATED) || mut->entry_exists(BETWEEN) ) {
        is_small = false;
      }
      
      if (!is_small) {
        ++mut_it;
        continue;
      }
    }
    
    cReferenceCoordinate start_coord = mut->get_reference_coordinate_start();
    cReferenceCoordinate end_coord = mut->get_reference_coordinate_end();
    
    cFlaggedRegions::regions_t contained_within = flagged_regions.regions_that_contain(mut->at(SEQ_ID), start_coord, end_coord);
    
    advance_it = true;
    
    if (contained_within.size() != 0) {
      
      if (verbose) {
        cout << endl << "Removing mutation:" << endl << "  " << *mut << endl;
        cout << "  Contained within MASK region(s):";
        for (cFlaggedRegions::regions_t::iterator region_it = contained_within.begin(); region_it != contained_within.end(); region_it++) {
          cout << " " << mut->at(SEQ_ID) << ":" << region_it->first << "-" << region_it->second;
        }
        cout << endl;
      }
      
      mut_it = this->remove(mut_it);
      advance_it = false;
    }
    
    if (advance_it) ++mut_it;
  }
  
  
  // Merge UN evidence into flagged regions
  diff_entry_list_t uns = get_list(make_vector<gd_entry_type>(UN));
  for (diff_entry_list_t::iterator un_it = uns.begin(); un_it != uns.end(); un_it++) {
    diff_entry_ptr_t& un = *un_it;
    flagged_regions.flag_region(un->at(SEQ_ID), from_string<uint32_t>(un->at(START)), from_string<uint32_t>(un->at(END)));
  }
  
  // Delete all evidence (including old UN entries)
  remove_group(EVIDENCE);
  
  // Add back UN evidence that includes original UN and MASKS
  std::list<std::string> seq_ids = flagged_regions.get_seq_ids();
  for (std::list<std::string>::iterator its=seq_ids.begin(); its != seq_ids.end(); its++) {
    cFlaggedRegions::regions_t regions = flagged_regions.all_regions(*its);
    for(cFlaggedRegions::regions_t::iterator it=regions.begin(); it!=regions.end(); it++ ) {
      cDiffEntry mask_entry(UN);
      mask_entry[SEQ_ID] = *its;
      mask_entry[START] = to_string<uint32_t>(it->first);
      mask_entry[END] = to_string<uint32_t>(it->second);
      this->add(mask_entry);
    }
  }
  
  // Let's fix the IDs to be in order
  this->reassign_unique_ids();
}
  
  
void cGenomeDiff::filter_to_within_region(cReferenceSequences& ref_seq_info, const string& region) {
  
  uint32_t target_id, start_pos, end_pos, insert_start, insert_end;
  ref_seq_info.parse_region(region, target_id, start_pos, end_pos);
  string seq_id = ref_seq_info[target_id].m_seq_id;
  
  // Create one flagged region
  cFlaggedRegions flagged_regions;
  flagged_regions.flag_region(seq_id, start_pos, end_pos);
  
  // Mask mutations by removing entries from the current GD file
  diff_entry_list_t::iterator mut_it = this->_entry_list.begin();
  
  bool advance_it(true);
  while (mut_it != this->_entry_list.end()) {
    
    diff_entry_ptr_t& mut = *mut_it;
    
    // ONLY mutations
    if (!mut->is_mutation()) {
      ++mut_it;
      continue;
    }
    
    cReferenceCoordinate start_coord = mut->get_reference_coordinate_start();
    cReferenceCoordinate end_coord = mut->get_reference_coordinate_end();
    
    cFlaggedRegions::regions_t contained_within = flagged_regions.regions_that_overlap(mut->at(SEQ_ID), start_coord, end_coord);
    
    // Remove if mutation does not overlap region of interest
    advance_it = true;
    if (contained_within.size() == 0) {
      mut_it = this->remove(mut_it);
      advance_it = false;
    }
    if (advance_it) ++mut_it;
  }

}

  
void cGenomeDiff::normalize_mutations(cReferenceSequences& ref_seq_info, Settings& settings, bool verbose)
{
  (void) verbose;
  
  // Pull settings variables
  int32_t AMP_size_cutoff = settings.size_cutoff_AMP_becomes_INS_DEL_mutation;

  // Convert all AMP to INS -- except the ones that we don't normalize
  //   so that INS/DEL normalization can take care of them
  diff_entry_list_t mut_list = this->mutation_list();
  for(diff_entry_list_t::iterator it=mut_list.begin(); it!=mut_list.end(); it++) {
    cDiffEntry& mut = **it;
    if (mut._type != AMP)
      continue;
    
    if ( mut.entry_exists("within") || mut.entry_exists("no_normalize") )
      continue;
      
    int32_t new_copy_number = from_string<uint32_t>(mut["new_copy_number"]);
    int32_t unit_size = from_string<int32_t>(mut[SIZE]);
    int32_t size = unit_size * (new_copy_number - 1);
    
    int32_t pos = from_string<uint32_t>(mut[POSITION]);
    // SKIP AMPS THAT GO PAST THE END OF THE SEQUENCE
    if (pos + size - 1 > static_cast<int32_t>(ref_seq_info.get_sequence_length(mut[SEQ_ID])))
      continue;
    
    mut._type = INS;
    string amped_seq = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos + unit_size - 1);
    mut[NEW_SEQ] = "";
    for(int32_t i=1; i< new_copy_number; i++){
      mut[NEW_SEQ] =  mut[NEW_SEQ] + amped_seq;
    }
    
    // note shift down by one position is correct
    // because INS are after this position, but AMP start at this position
    mut["position"] = to_string<int32_t>(pos - 1);
    mut.erase("new_copy_number");
    mut.erase("size");
  }
  
  MutationPredictor mp(ref_seq_info);
  Summary summary; // don't pass these through currently
  mp.normalize_and_annotate_tandem_repeat_mutations(settings, summary, *this);
  mp.normalize_INS_to_AMP(settings, summary, *this);

}

cGenomeDiff cGenomeDiff::check(cGenomeDiff& ctrl, cGenomeDiff& test, bool verbose)
{
  bool (*comp_fn) (const diff_entry_ptr_t&, const diff_entry_ptr_t&) = diff_entry_ptr_compare_sort;
  typedef set<diff_entry_ptr_t, bool(*)(const diff_entry_ptr_t&, const diff_entry_ptr_t&)> diff_entry_set_t;
  
  //Isolate control and test mutations into sets for quick lookup.
  diff_entry_list_t muts = ctrl.mutation_list();
  for(diff_entry_list_t::iterator it = muts.begin(); it != muts.end(); it++) {
    (**it).erase("phylogeny_id");
    (**it).erase("population_id");
    (**it).erase("unique");
  }
  
  diff_entry_set_t ctrl_muts(comp_fn);
  copy(muts.begin(), muts.end(), inserter(ctrl_muts, ctrl_muts.begin()));
  
  muts = test.mutation_list();
  for(diff_entry_list_t::iterator it = muts.begin(); it != muts.end(); it++) {
    (**it).erase("phylogeny_id");
    (**it).erase("population_id");
    (**it).erase("unique");
  }
  
  diff_entry_set_t test_muts(comp_fn);
  copy(muts.begin(), muts.end(), inserter(test_muts, test_muts.begin()));
  
  if (verbose) {
    printf("\tComparing %u control mutations versus %u test mutations.\n\n",
           static_cast<unsigned int>(ctrl_muts.size()), static_cast<unsigned int>(test_muts.size()));
  }
  
  /* Combine alike mutations, however we may lose information like supporting evidence IDs
   * which we will search ctrl_muts and test_muts for.
   */
  
  
  diff_entry_set_t uniq_muts(comp_fn);
  std::set_union(
                 ctrl_muts.begin(),
                 ctrl_muts.end(),
                 test_muts.begin(),
                 test_muts.end(),
                 inserter(uniq_muts, uniq_muts.begin()),
                 comp_fn
                 );
  
  uint32_t n_tp = 0, n_fn = 0, n_fp = 0;
  
  cGenomeDiff ret_val;
  ret_val.metadata = test.metadata;
  
  for (diff_entry_set_t::iterator it = uniq_muts.begin(); it != uniq_muts.end(); ++it) {
    assert(ctrl_muts.count(*it) || test_muts.count(*it));
    
    string key = "";
    diff_entry_list_t evidence;
    diff_entry_set_t::iterator it_ctrl = ctrl_muts.find(*it);
    diff_entry_set_t::iterator it_test = test_muts.find(*it);
    bool in_ctrl = (it_ctrl != ctrl_muts.end());
    bool in_test = (it_test != test_muts.end());
    if (in_ctrl && in_test) {
      key = "TP";
      ++n_tp;
      evidence = test.mutation_evidence_list(**it_test);
      if (evidence.empty()) {
        evidence = ctrl.mutation_evidence_list(**it_ctrl);
      }
    }
    else if (in_ctrl && !in_test) {
      key = "FN";
      ++n_fn;
      evidence = ctrl.mutation_evidence_list(**it_ctrl);
    }
    else if (!in_ctrl && in_test) {
      key = "FP";
      ++n_fp;
      evidence = test.mutation_evidence_list(**it_test);
    } 
    
    if (verbose) {
      string temp = "";
      if (key == "TP") temp = "[True  Positive]:\t";
      if (key == "FN") temp = "[False Negative]:\t";
      if (key == "FP") temp = "[False Positive]:\t";
      if (key == "")   temp = "[ERROR]         :\t";
      cout << "\t\t"<< temp << **it << endl;
    }
    
    (**it)["compare"] = key;
    (**it)._evidence.clear();
    
    //Add supporting evidence and assign new IDs to the mutation.
    diff_entry_ptr_t mut = ret_val.add(**it);
    
    
    /* Something can go awry here
    if (evidence.size()) {
      for (diff_entry_list_t::iterator jt = evidence.begin(); jt != evidence.end(); ++jt) {
        (**jt)["compare"] = key;
        mut->_evidence.push_back(ret_val.add(**jt)->_id);
      }
    } else {
      mut->_evidence.push_back(".");
    }
    */
  }
  
  /*
  if (verbose) {
    printf("\tUpdating mutation's evidence.\n"); 
  }
  */
  
  //Add TP|FN|FP header info.
  string value = "";
  sprintf(value, "%u|%u|%u", n_tp, n_fn, n_fp);
  ret_val.add_breseq_data("TP|FN|FP", value);
  
  printf("\t#=TP|FN|FP	%s \t for %s versus %s \n",
         value.c_str(),
         ctrl.get_file_name().c_str(),
         test.get_file_name().c_str());
  
  return ret_val;
}

  
// Returns true if two junction sequences are considered equivalent
//  -- subsequence or subsequence of reverse complement
bool equivalent_junction_sequences(string s1, string s2) {
  if (s1.size() < s2.size()) {
    
    if (s2.find(s1) != string::npos ) {
      return true;
    }
    if (s2.find(reverse_complement(s1)) != string::npos) {
      return true;
    }
  }
  else {
    if (s1.find(s2) != string::npos ) {
      return true;
    }
    if (s1.find(reverse_complement(s2)) != string::npos) {
      return true; 
    }
  }
  return false;
}
  
typedef map<string, diff_entry_ptr_t> jc_data_t;  
  
// Helper function that returns keys of all equivalent junctions
vector<string> equivalent_junction_keys(jc_data_t& jcs, string& key)
{
  vector<string> matching_keys;
  
  for (jc_data_t::iterator it = jcs.begin(); it != jcs.end(); it++) {
    
    if (equivalent_junction_sequences(key, it->first))
      matching_keys.push_back(it->first);
  }
  
  return matching_keys;
}

cGenomeDiff cGenomeDiff::check_evidence(cReferenceSequences& sequence, 
                                           uint32_t buffer,
                                           uint32_t shorten_length,
                                           cGenomeDiff& ctrl,
                                           cGenomeDiff& test,
                                           bool jc_only_accepted,
                                           bool verbose) {
  
  //TODO currently only compares JC evidence.
  cGenomeDiff ret_val;
  ret_val.metadata = test.metadata;
  
  
  // START JC Evidence Block
  //
  // Conceptually we build up two sets containing equivalent junction sequences.
  // One is from the control set and one is from the test set.
  // 
  // * The control set must have no equivalent junctions in it.
  //
  // * The test set isn't penalized for predicting an equivalent junction many times
  //   Subsequent appearances of the same junction are not forwarded.
  
  // This keeps track of all unique sequences.
  // We assume that each sequence will only occur ONCE in each genome diff file.
  // Otherwise you can have more True Positives than the total that should be possible if you
  // predict the same junction twice...
  
  jc_data_t ctrl_jc;
  
  diff_entry_list_t ctrl_list = ctrl.get_list(make_vector<gd_entry_type>(JC));
  ctrl_list.remove_if(cDiffEntry::field_exists("circular_chromosome")); 
  
  ////////////////////////////
  //      CONTROL list      //
  ////////////////////////////
  
  for (diff_entry_list_t::iterator it = ctrl_list.begin(); it != ctrl_list.end(); ++it) {
    cDiffEntry& jc = **it;
    
    string jc_segment = CandidateJunctions::construct_junction_sequence(sequence, jc, buffer, true);
    jc["segment"] = jc_segment;
    ASSERT(jc_segment.size(), "Could not locate JC sequence for: " + (*it)->as_string());
    
    // This properly deals with reverse-complements and subsequences
    vector<string> equivalent_segments = equivalent_junction_keys(ctrl_jc, jc_segment);
    if ( equivalent_segments.size() > 0 ) {
      ERROR("Duplicate junction sequence in control data set for entry:\n" + jc.as_string() + "\n" + ctrl_jc[jc_segment]->as_string());
    }
    
    // We have to shorten the control segments to allow for this situation to be equivalent...
    // GATCTAGTCATGCTAC
    //  ATCTAGTCATGCTACG
    
    jc_segment = jc_segment.substr(shorten_length, jc_segment.size()-2*shorten_length);
    
    ctrl_jc[jc_segment] = *it;
  }
  
  
  /////////////////////////
  //      TEST list      //
  /////////////////////////
  
  // For recovering full information about a junction from the set
  jc_data_t test_jc;
  
  diff_entry_list_t test_list = test.get_list(make_vector<gd_entry_type>(JC));
  test_list.remove_if(cDiffEntry::field_exists("circular_chromosome")); 
  if (jc_only_accepted) test_list.remove_if(cDiffEntry::field_exists("reject")); 
  
  int32_t i = 0;
  for (diff_entry_list_t::iterator it = test_list.begin(); it != test_list.end(); ++it) {
    
    cDiffEntry& jc = **it;
    
    string jc_segment = CandidateJunctions::construct_junction_sequence(sequence, jc, buffer, true);
    jc["segment"] = jc_segment;
    ASSERT(jc_segment.size(), "Could not locate JC sequence for: " + (*it)->as_string());
    
    vector<string> equivalent_segments = equivalent_junction_keys(test_jc, jc_segment);
    
    for (vector<string>::iterator its=equivalent_segments.begin(); its != equivalent_segments.end(); its++) {
      
      diff_entry_ptr_t prev_jc = test_jc[*its];
      
      WARN("Duplicate junction sequence in test data set for entry:\n" + jc.as_string() + "\n" + ctrl_jc[jc_segment]->as_string());
      
      if (verbose) {
        cerr << "*** Merged two junctions:" << endl;
        cerr << jc << endl;
        cerr << "AND:" << endl;
        cerr << *(prev_jc) << endl;
      }
      
      // Save the max score with the new item
      if (n(jc["score"]) < n((*prev_jc)["score"])) {
        jc["score"] = (*prev_jc)["score"];
      }
      
      if (verbose) {
        cerr << "Result:" << endl;
        cerr << jc << endl;
      }
      
      test_jc.erase(*its);
    }
    
    test_jc[jc_segment] = *it;
  }
  
  //////////////////////////////////////
  //     Assignment of TP, FP, FN     //
  //////////////////////////////////////
  
  uint32_t n_tp = 0, n_fn = 0, n_fp = 0;
  
  // Create the new item, which must either be a true-positive or a false-positive!
  
  for (jc_data_t::iterator it = test_jc.begin(); it != test_jc.end(); it++) {
    
    string test_junction_seq = it->first;
    cDiffEntry& jc = *(it->second);
    
    cDiffEntry new_item = jc.to_spec();
    new_item["segment"] = test_junction_seq;
    
    if (jc.count("score")) {
      new_item["score"] = jc["score"];
    } else if (jc.count("neg_log10_pos_hash_p_value")) {
      new_item["score"] = jc["neg_log10_pos_hash_p_value"];
    } else {
      new_item["score"] = "9999999";
    }
    
    vector<string> equivalent_segments = equivalent_junction_keys(ctrl_jc, test_junction_seq);
    
    // One item found = TP
    if (equivalent_segments.size() == 1) {    
      new_item["compare"] = "TP";
      ++n_tp;
      // delete from control set
      ctrl_jc.erase(equivalent_segments[0]);
      // Zero items found = FP
    } else if (equivalent_segments.size() == 0) {
      new_item["compare"] = "FP";
      ++n_fp;
    }
    // More than one item found = ERROR! (Should be ruled out above.)
    else {
      ERROR("More than one match in control found for junction:\n" + jc.as_string());
    }
    
    ret_val.add(new_item);
  }
  
  // Now iterate through remaining control items, which are false-negatives
  for (jc_data_t::iterator it = ctrl_jc.begin(); it != ctrl_jc.end(); it++) {
    string test_junction_seq = it->first;
    cDiffEntry& jc = *(it->second);
    
    cDiffEntry new_item = jc.to_spec();
    new_item["segment"] = test_junction_seq;
    new_item["compare"] = "FN";
    new_item["score"] = "0"; // Control items don't have a score...
    ++n_fn;
    ret_val.add(new_item);
  }
  
  //Add TP|FN|FP header info.
  string value = "";
  sprintf(value, "%u|%u|%u", n_tp, n_fn, n_fp);
  ret_val.add_breseq_data("TP|FN|FP", value);
  
  printf("\t#=TP|FN|FP	%s \t for %s versus %s \n",
         value.c_str(),
         ctrl.get_file_name().c_str(),
         test.get_file_name().c_str());
  
  return ret_val;
}

// @JEB: This is only partially implemented -- it adds JC items for mutations involving these
void cGenomeDiff::mutations_to_evidence(cReferenceSequences &ref_seq, bool remove_mutations)
{
  diff_entry_list_t muts = mutation_list();
  this->remove_group(EVIDENCE);
  this->remove_group(VALIDATION);
  for(diff_entry_list_t::iterator it=muts.begin(); it!=muts.end(); ++it) {
    cDiffEntry& de = **it;
    if (de._type == MOB) {
      // Return the actual copy of the sequence feature found...
      cFeatureLocation repeat_feature;
      string repeat_seq_id;
      string requested_repeat_region;
      ref_seq.repeat_family_sequence(de["repeat_name"], +1, de.entry_exists("mob_region") ? &de["mob_region"] : NULL, &repeat_seq_id, &repeat_feature);
      
      cDiffEntry de1;
      de1._type = JC;
      de1["side_1_seq_id"] = de["seq_id"];
      de1["side_1_position"] = s(n(de["position"]) + n(de["duplication_size"]) - 1);
      de1["side_1_strand"] = "-1";
      de1["side_2_seq_id"] = repeat_seq_id;
      int32_t strand = repeat_feature.get_strand() * n(de["strand"]);
      de1["side_2_position"] = (strand > 0) ? s(repeat_feature.get_start_1()) : s(repeat_feature.get_end_1());
      de1["side_2_strand"] = s(strand);
      de1["overlap"] = "0";
      (*it)->_evidence.push_back(this->add(de1)->_id);
      
      cDiffEntry de2;
      de2._type = JC;
      de2["overlap"] = "0";
      de2["side_1_seq_id"] = de["seq_id"];
      de2["side_1_position"] = s(n(de["position"]));
      de2["side_1_strand"] = "1";
      de2["side_2_seq_id"] = repeat_seq_id;
      de2["side_2_position"] = (strand > 0) ? s(repeat_feature.get_end_1()) : s(repeat_feature.get_start_1());
      de2["side_2_strand"] = s(-strand);
      (*it)->_evidence.push_back(this->add(de2)->_id);
      
    } else if (de._type == DEL) {
      
      cDiffEntry de1;
      de1._type = JC;
      de1["overlap"] = "0";
      de1["side_1_seq_id"] = de["seq_id"];
      de1["side_1_position"] = s(n(de["position"])-1);
      de1["side_1_strand"] = "-1";
      de1["side_2_seq_id"] = de["seq_id"];
      de1["side_2_position"] = s(n(de["position"])+n(de["size"]));
      de1["side_2_strand"] = "1";
      (*it)->_evidence.push_back(this->add(de1)->_id);
      
    }
  }
  if (remove_mutations) {
    this->remove_group(MUTATIONS);
  }
}
  

void cGenomeDiff::write_jc_score_table(cGenomeDiff& compare, string table_file_path, bool verbose) {
  //assert(compare.metadata.breseq_data.count("TP|FN|FP"));

  typedef map<float, map<string, uint32_t> > table_t;
  table_t table;
  
  diff_entry_list_t jc = compare.get_list(make_vector<gd_entry_type>(JC));
  double max_score = 0;
  uint32_t total_gold_standard_predictions = 0;
  for (diff_entry_list_t::iterator it = jc.begin(); it != jc.end(); ++it) {
    if (!(*it)->count("score")) {
      cerr << "No score value for: " + (*it)->as_string() << endl;
      continue;
    }
    double score = from_string<double>((**it)["score"]);
    //score = roundp<10>(score);
    max_score = max(max_score, score);

    assert((*it)->count("compare"));
    string compare = (**it)["compare"];

    if ( (compare == "TP") || (compare == "FN") ) {
      total_gold_standard_predictions++; 
    }
    if (compare == "FN") continue;
    
    if (!table.count(score)) {
      table[score]["TP"] = 0;
      table[score]["FN"] = 0;
      table[score]["FP"] = 0;
    }
  
    table[score][compare]++;
  }


  ofstream out(table_file_path.c_str());
  ASSERT(out, "Could not write to file: " + table_file_path);
  
  out << "score" << '\t' << "TP" << '\t' << "FN" << '\t' << "FP" << endl;
  if (verbose) {
    cerr << "\t\tscore" << '\t' << "TP" << '\t' << "FN" << '\t' << "FP" << endl;
  }
  uint32_t n_tp = 0, n_fn = 0, n_fp = 0;
  
  //TODO @GRC for sorting tophat scoring parameters, remove if/when needed.
  bool is_tophat_scoring = compare.metadata.author == "tophat" ? true : false;
  if (is_tophat_scoring) {
    for (table_t::reverse_iterator it = table.rbegin(); it != table.rend(); it++) {
      double i = it->first;
      //double i = 0; i <= max_score; i += .1f) {
      
      if (table.count(i)) {
        n_tp += table[i]["TP"];
        // The total number of TP and FN at a given score is equal to the total minus the number of TP up to that point.
        n_fn =  total_gold_standard_predictions - n_tp;
        n_fp += table[i]["FP"];

        out << i << '\t' << n_tp << '\t' << n_fn << '\t' << n_fp << endl;
        if (verbose) {
          cerr << "\t\t" << i << '\t' << n_tp << '\t' << n_fn << '\t' << n_fp << endl;
        }

      }

    }
  } else {
    for (table_t::iterator it = table.begin(); it != table.end(); it++) {
      double i = it->first;
      //double i = 0; i <= max_score; i += .1f) {
      
      if (table.count(i)) {
        n_tp += table[i]["TP"];
        // The total number of TP and FN at a given score is equal to the total minus the number of TP up to that point.
        n_fn =  total_gold_standard_predictions - n_tp;
        n_fp += table[i]["FP"];

        out << i << '\t' << n_tp << '\t' << n_fn << '\t' << n_fp << endl;
        if (verbose) {
          cerr << "\t\t" << i << '\t' << n_tp << '\t' << n_fn << '\t' << n_fp << endl;
        }

      }

    }
  }
  out.close();
}


// Used to add frequency_base-name columns to the master gd by
// finding equivalent mutations in 
void cGenomeDiff::tabulate_frequencies_from_multiple_gds(
                                                         cGenomeDiff& master_gd, 
                                                         vector<cGenomeDiff>& gd_list, 
                                                         vector<string> &title_list,
                                                         bool phylogeny_id_aware,
                                                         bool verbose
                                                         )
{  
  // Create a list of unique titles, which may require some renaming
  // so that every one has a unique key in the resulting Genome Diff entries
  set<string> used_titles; // for reassigning duplicate names
  title_list.resize(0);
  
  for (vector<cGenomeDiff>::iterator it = gd_list.begin(); it != gd_list.end(); it++) {
    
    // Assign unique names to every file and warn if names were changed.
    string this_title = it->get_title();
    uint32_t counter=0;
    while (used_titles.find(this_title) != used_titles.end() ) {
      this_title = it->get_title() + "_" + to_string(++counter);
    }
    
    if (this_title != it->get_title()) {
      WARN("Duplicate title changed from '" + it->get_title() + "' to '" + this_title + "'\nFor genome diff file with path: " + it->get_file_path());
    }
    
    title_list.push_back(this_title);
    used_titles.insert(this_title);
  }
  
  
  vector<diff_entry_list_t> mut_lists;
  for (vector<cGenomeDiff>::iterator it = gd_list.begin(); it != gd_list.end(); it++) {
    mut_lists.push_back(it->mutation_list());
  }

  diff_entry_list_t de_list = master_gd.mutation_list();

  if (verbose) cout << "Tabulating frequencies for mutation..." << endl;
  
  for (diff_entry_list_t::iterator it = de_list.begin(); it != de_list.end(); it++) { 
    
    diff_entry_ptr_t& this_mut = *it;
    uint32_t this_mut_position = from_string<uint32_t>((*this_mut)[POSITION]);
    
    if (!phylogeny_id_aware) {
      this_mut->erase("phylogeny_id");
      this_mut->erase("population_id");
    }
    
    if (verbose) cout << ">> Master Mutation" << endl<< this_mut->as_string() << endl;
    
    // for each genome diff compared
    for (uint32_t i=0; i<mut_lists.size(); i++) { 
      
      string freq_key = "frequency_" + title_list[i];
      (*this_mut)[freq_key] = "0";
      
      diff_entry_list_t& mut_list = mut_lists[i];
            
      // We have some problems when there are multiple INS after the same position in a genomediff...
      // they get merged to one, e.g.
      // INS	177	1750	T7_WT_Genome	24198	T	frequency=0.1310
      // INS	178	1751	T7_WT_Genome	24198	T	frequency=0.0250
      /*
      while (mut_list.size() && (from_string<uint32_t>((*mut_list.front())[POSITION]) < this_mut_position)) {
        //cout << (*mut_list.front())[POSITION] << " < " << this_mut_position << endl;
        mut_list.pop_front();
      }
      */ 
      if (mut_list.size() == 0) 
        continue; 
      // End code for multiple INS problem.  
    
      // for top mutation in this genomedff (they are sorted by position)
      diff_entry_ptr_t check_mut = mut_list.front();
      
      // There should never be two of the same mutation in a file or it screws things up
      diff_entry_list_t::iterator mut_list_it = mut_list.begin();
            
      if (mut_list_it != mut_list.end()) {
        mut_list_it++;
        if (mut_list_it != mut_list.end()) {
          diff_entry_ptr_t next_mut = *mut_list_it;
          
          //cout << endl << "Check mut:" << check_mut->as_string() << endl << "Next mut:" << next_mut->as_string() << endl;
          
          ASSERT( !(*next_mut == *check_mut), "Identical diff entry items found in file:\n" + gd_list[i].get_file_path() + "\n1>>\n" + check_mut->as_string() + "\n2>>\n" + next_mut->as_string() + "\n");
        }
      }
      
      
      if (verbose)
        cout << ">" << title_list[i] << endl << check_mut->as_string() << endl;
      
      // Prevent keeping things separate based on phylogeny id
      if (!phylogeny_id_aware) {
        check_mut->erase("phylogeny_id");
        check_mut->erase("population_id");
      }
      
      // we found the exact same mutation
      if ( (check_mut.get() != NULL) && (*check_mut == *this_mut) ) {
        
        if (check_mut->count(FREQUENCY))
          (*this_mut)[freq_key] = (*check_mut)[FREQUENCY];
        else
          (*this_mut)[freq_key] = "1";
        
        // remove the item
        mut_list.pop_front();
        continue;
      }
      
      // we didn't find the mutation, so we might need to consider it deleted or unknown...
      if (gd_list[i].mutation_deleted(*this_mut)) {
        (*this_mut)[freq_key] = "D";
        continue;
      }
      
      if (gd_list[i].mutation_unknown(*this_mut)) {
        (*this_mut)[freq_key] = "?";
        continue;
      }
    }
  }

}

  
// This output is meant to be a dump to be parsable in R
// it includes extra metadata on every row to identify the file, population, time, clone, etc.
void cGenomeDiff::write_tsv(
                            string& output_csv_file_name,
                            vector<cGenomeDiff>& gd_list,
                            bool verbose
                            )
{
  (void)verbose;
  
  set<string> key_set;
  
  // Go through all entries and merge keys into one big map
  // Go through each row and write everything, with blanks for when keys are missing
  
  for (vector<cGenomeDiff>::iterator gd_it = gd_list.begin(); gd_it != gd_list.end(); gd_it++)  {
    
    diff_entry_list_t de_list = gd_it->get_list();
    for (diff_entry_list_t::iterator de_it = de_list.begin(); de_it != de_list.end(); de_it++)  {
      
      (**de_it)["type"] = gd_entry_type_lookup_table[(*de_it)->_type];
      (**de_it)["title"] = gd_it->metadata.title;
      (**de_it)["treatment"] = gd_it->metadata.treatment;
      (**de_it)["population"] = gd_it->metadata.population;
      (**de_it)["time"] = to_string<double>(gd_it->metadata.time);
      (**de_it)["clone"] = gd_it->metadata.clone;

      
      (**de_it)["mutator_status"] = (gd_it->metadata.breseq_data.find("MUTATOR_STATUS") != gd_it->metadata.breseq_data.end()) ? gd_it->metadata.breseq_data["MUTATOR_STATUS"] : "";

      
      for (diff_entry_map_t::iterator it = (*de_it)->begin(); it != (*de_it)->end(); it++) {
        if (it->first[0] != '_') {
          key_set.insert(it->first);
        }
      }
    }
  }
  
  vector<string> key_list( key_set.begin(), key_set.end() );
  
  ofstream output_file(output_csv_file_name.c_str());
  output_file <<  join(key_list, "\t") << endl;

  for (vector<cGenomeDiff>::iterator gd_it = gd_list.begin(); gd_it != gd_list.end(); gd_it++)  {
  
    diff_entry_list_t de_list = gd_it->get_list();
    for (diff_entry_list_t::iterator de_it = de_list.begin(); de_it != de_list.end(); de_it++)  {
      
      bool first_time = true;
      for (vector<string>::iterator it = key_list.begin(); it != key_list.end(); it++) {
        if (!first_time) output_file << "\t";
        output_file << ( (*de_it)->entry_exists(*it) ? (*de_it)->get(*it) : "");
        first_time = false;
      }
      output_file << endl;
    }
  }
  
}
  
// Convert GD file to VCF file
//
// VCF Format https://github.com/amarcket/vcf_spec
// This function was written by consulting v4.2
// (which gives the example below)
//
// ##fileformat=VCFv4.2
// ##fileDate=20090805
// ##source=myImputationProgramV3.1
// ##reference=file:///seq/references/1000GenomesPilot-NCBI36.fasta
// ##contig=<ID=20,length=62435964,assembly=B36,md5=f126cdf8a6e0c7f379d618ff66beb2da,species="Homo sapiens",taxonomy=x>
// ##phasing=partial
// ##INFO=<ID=NS,Number=1,Type=Integer,Description="Number of Samples With Data">
// ##INFO=<ID=DP,Number=1,Type=Integer,Description="Total Depth">
// ##INFO=<ID=AF,Number=A,Type=Float,Description="Allele Frequency">
// ##INFO=<ID=AA,Number=1,Type=String,Description="Ancestral Allele">
// ##INFO=<ID=DB,Number=0,Type=Flag,Description="dbSNP membership, build 129">
// ##INFO=<ID=H2,Number=0,Type=Flag,Description="HapMap2 membership">
// ##FILTER=<ID=q10,Description="Quality below 10">
// ##FILTER=<ID=s50,Description="Less than 50% of samples have data">
// ##FORMAT=<ID=GT,Number=1,Type=String,Description="Genotype">
// ##FORMAT=<ID=GQ,Number=1,Type=Integer,Description="Genotype Quality">
// ##FORMAT=<ID=DP,Number=1,Type=Integer,Description="Read Depth">
// ##FORMAT=<ID=HQ,Number=2,Type=Integer,Description="Haplotype Quality">
// #CHROM POS ID REF ALT QUAL FILTER INFO FORMAT NA00001 NA00002 NA00003
//   20 14370 rs6054257 G A 29 PASS NS=3;DP=14;AF=0.5;DB;H2 GT:GQ:DP:HQ 0|0:48:1:51,51 1|0:48:8:51,51 1/1:43:5:.,.
//   20 17330 . T A 3 q10 NS=3;DP=11;AF=0.017 GT:GQ:DP:HQ 0|0:49:3:58,50 0|1:3:5:65,3 0/0:41:3
//   20 1110696 rs6040355 A G,T 67 PASS NS=2;DP=10;AF=0.333,0.667;AA=T;DB GT:GQ:DP:HQ 1|2:21:6:23,27 2|1:2:0:18,2 2/2:35:4
//   20 1230237 . T . 47 PASS NS=3;DP=13;AA=T GT:GQ:DP:HQ 0|0:54:7:56,60 0|0:48:4:51,51 0/0:61:2
//   20 1234567 microsat1 GTC G,GTCT 50 PASS NS=3;DP=9;AA=G GT:GQ:DP 0/1:35:4 0/2:17:2 1/1:40:3
//
// IMPORTANT: Although it doesn't seem to specify -- ALL columns must be separated by TABS, not spaces
  
void cGenomeDiff::write_vcf(const string &vcffile, cReferenceSequences& ref_seq_info)
{  
  ofstream output( vcffile.c_str() );
  output << "##fileformat=VCFv4.1" << endl;
  output << "##fileDate" << endl;
  output << "##source=breseq_GD2VCF_converterter" << endl;
  //output << "##reference=" << endl;
  
  
  // Write contig information.
  
  for(cReferenceSequences::iterator it=ref_seq_info.begin(); it!=ref_seq_info.end(); it++) {
    output << "##contig=<ID=" << it->m_seq_id << ",length=" << it->get_sequence_length() << ">" << endl;
  }
  
  output << "##INFO=<ID=AF,Number=A,Type=Float,Description=\"Allele Frequency\">" << endl;
  output << "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">" << endl;

  
  // Write header line
  //output << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tsample" << endl;
  output << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO" << endl;

  diff_entry_list_t muts = mutation_list();  
  for (diff_entry_list_t::iterator it=muts.begin(); it!=muts.end(); it++) {

    cDiffEntry& mut = **it;
    uint32_t pos = from_string<uint32_t>(mut[POSITION]);
    
    string chrom = mut[SEQ_ID];
    string id = ".";
    string ref;
    string alt;
    string qual = ".";
    string filter = ".";
    string info;
    string format = "GT";
    string sample = "1/1";
    
    // For now, only allele frequency info field
    double freq = 1.0;
    
    if (mut.entry_exists(FREQUENCY)) {
        freq = from_string<double>(mut[FREQUENCY]);
    }
    string AF = formatted_double(freq, 4).to_string(); // allele frequency
    info = "AF=" + AF;
    
    switch (mut._type) 
    {
      case SNP:
      { 
        alt = mut[NEW_SEQ];
        ref = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos);
        
        // Carry forward qulity from related RA evidence
        diff_entry_list_t ev = mutation_evidence_list(mut);
        if (ev.size() == 1) 
          qual = ev.front()->count("genotype_quality") ? (*ev.front())["genotype_quality"] : ".";

      } break;
        
      case SUB:
      {
        // We know Ref seq is not just a "." in this case.
        
        alt = mut[NEW_SEQ];
        const uint32_t& size = from_string<uint32_t>(mut[SIZE]);
        ref = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos + size - 1);

        // Carry forward qulity from related RA evidence
        // @JEB should do something with multiple evidence
        diff_entry_list_t ev = mutation_evidence_list(mut);
        if (ev.size() == 1) 
          qual = ev.front()->count("genotype_quality") ? (*ev.front())["genotype_quality"] : ".";
        
      } break;
        
      case INS:
      {          
        // Ref has to be something - so take base insertion was after
        // Complication: It could be inserted at the beginning of the sequence.
        // In this case, we take the base after.
        
        bool before_base = (pos != 0);
        if (!before_base) pos++;
        ref = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos);

        // Correct position of first base shown, if necessary
        
        alt = before_base ? ref + mut[NEW_SEQ] : mut[NEW_SEQ] + ref;
        
        // Carry forward quality from related RA evidence
        // @JEB should do something with multiple evidence
        diff_entry_list_t ev = mutation_evidence_list(mut);
        if (ev.size() == 1) 
          qual = ev.front()->count("genotype_quality") ? (*ev.front())["genotype_quality"] : ".";
        
      } break;
        
      case DEL:
      {
        // Complication: Alt has to be something (not .)
        // So, take first base before deletion or after if it is the first base
        
        const uint32_t& size = from_string<uint32_t>(mut[SIZE]);
        ref = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos + size - 1);
        
        bool before_base = (pos != 1);
        alt = before_base ? ref_seq_info.get_sequence_1(mut[SEQ_ID], pos-1, pos-1) : ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos);
        ref = before_base ? alt + ref : ref + alt;

        // Correct position of first base shown, if necessary
        if (before_base) pos--;
        
        // Carry forward quality from related RA evidence
        // @JEB should do something with multiple evidence
        diff_entry_list_t ev = mutation_evidence_list(mut);
        if (ev.size() == 1) 
          qual = ev.front()->count(CONSENSUS_SCORE) ? (*ev.front())[CONSENSUS_SCORE] : ".";
        
      } break;
        
      case AMP:
      {        
        const uint32_t& size = from_string<uint32_t>(mut[SIZE]);
        
        //Build duplicate sequence
        for (uint32_t i = 0; i < from_string<uint32_t>(mut[NEW_COPY_NUMBER]); i++)
          alt.append(ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos+size-1));
        ASSERT(!alt.empty(), "Duplicate sequence is empty. You may have specified an AMP with a new copy number of 1.");
        
        ref = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos+size-1);

        // @JEB should do something with multiple evidence
        diff_entry_list_t ev = mutation_evidence_list(mut);
        if (ev.size() == 1) qual = ev.front()->count("genotype_quality") ? (*ev.front())["genotype_quality"] : ".";
        
      } break;
        
      case INV:
      {        
        WARN("INV: mutation type not handled yet");
      } break;
        
      case MOB:
      {
        // Ref has to be something - so take first base before deletion
        // Or after if there is none before
        
        bool before_base = (pos != 1);
        ref = before_base ? ref_seq_info.get_sequence_1(mut[SEQ_ID], pos-1, pos) : ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos + 1);
        //ref = before_base ? alt + ref : ref + alt;
        
        // This includes the IS and all adjacent duplicated or deleted nucleotides  
        string new_seq_string = mob_replace_sequence(ref_seq_info, mut);
        
        // The position of a MOB is the first position that is duplicated
        // Inserting at the position means we have to copy the duplication
        // in FRONT OF the repeat sequence
        
        string duplicate_sequence;
        int32_t iDupLen = from_string<int32_t>(mut["duplication_size"]);
        if (iDupLen < 0) {
          cerr << "Warning: MOB with negative target site insertion not handled. Ignoring:" << endl << mut << endl;
          break;
        }
        if (iDupLen > 0) {
          duplicate_sequence = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos + iDupLen - 1);
        }
        
        // Add on the duplicated sequence.  This happens AFTER
        // we have inserted any insertions.
        new_seq_string = duplicate_sequence + new_seq_string;        
        
        if (before_base) 
          alt = ref + duplicate_sequence + new_seq_string;
        else
          alt = duplicate_sequence + new_seq_string + ref;
        
        // Correct position of first base shown, if necessary
        if (before_base) pos--;
        
      } break;
        
      case CON:
      {        
        uint32_t size = from_string<uint32_t>(mut[SIZE]);
        
        uint32_t replace_target_id, replace_start, replace_end;
        ref_seq_info.parse_region(mut["region"], replace_target_id, replace_start, replace_end);
        ASSERT(replace_start != replace_end, "Cannot process CON mutation with end == start. ID:" + mut._id);
        
        int8_t strand = (replace_start < replace_end) ?  +1 : -1;
        
        if (strand == -1)
          swap(replace_start, replace_end);
        
        // @JEB: correct here to look for where the replacing_sequence is in the original ref_seq_info.
        // This saves us from possible looking at a shifted location...
        alt = ref_seq_info[replace_target_id].get_sequence_1(replace_start, replace_end);
        
        if (strand == -1)
          alt = reverse_complement(alt);
        
        ref = ref_seq_info.get_sequence_1(mut[SEQ_ID], pos, pos + size - 1);
        
      } break;
        
      default:
        WARN("Can't handle mutation type: " + to_string(mut._type) + "\nIt will be skipped.");
    }
    
    //output << chrom << "\t" << pos << "\t" << id << "\t" << ref << "\t" << alt << "\t" << qual << "\t" << filter << "\t" << info << "\t" << format << "\t" << sample << endl;
    
        output << chrom << "\t" << pos << "\t" << id << "\t" << ref << "\t" << alt << "\t" << qual << "\t" << filter << "\t" << info << endl;
  }
  
  output.close();
}
  
// Convert GD file to GVF file
void cGenomeDiff::write_gvf(const string &gvffile, cReferenceSequences& ref_seq_info, bool snv_only)
{  

  diff_entry_list_t diff_entry_list = this->get_list();
  diff_entry_list_t::iterator it = diff_entry_list.begin();
  
  // Stores the features
  vector< vector<string> > features;
  vector< vector<string> > featuresGVF;
  
  // We only write entries for mutations
  diff_entry_list_t mut_list = this->mutation_list();
  
  // Processes the features
  // gvf[0]: ID of reference
  // gvf[1]: Source
  // gvf[2]: Type
  // gvf[3]: Start
  // gvf[4]: End
  // gvf[5]: Score
  // gvf[6]: Strand
  // gvf[7]: Phase
  // gvf[8]: Attributes    
  
  for (diff_entry_list_t::iterator it=mut_list.begin(); it != mut_list.end(); it++)
  {
    cDiffEntry& de = **it;
    
    vector<string> gvf(9,"");
    
    for( int j=5; j<8; j++ ){
      gvf[j] = ".";
    }
    
    // Common to all entries
    // SeqID
    gvf[0] = de[SEQ_ID];
    // Source
    gvf[1] = "breseq";
    // Position
    gvf[3] = de[POSITION];
    
    if( de._type == SNP )
    {
      // Type
      gvf[2] = "SNV";
      // End
      gvf[4] = de[POSITION];
      // Strand
      gvf[6] = "+";
      // Attributes - Reference base
      string ref_base = ref_seq_info.get_sequence_1(de[SEQ_ID], from_string(de[POSITION]),  from_string(de[POSITION]));
      gvf[8].append(";Reference_seq=").append( ref_base );
      // Attributes - New base
      gvf[8].append("Variant_seq=").append( de[NEW_SEQ] );
      
      diff_entry_list_t ev_list = mutation_evidence_list(de);
      ASSERT(ev_list.size() == 1, "Did not find RA evidence supporting SNP\n" + to_string(de))
      cDiffEntry& ev = *(ev_list.front());
      
      // Score
      gvf[5] = ev[CONSENSUS_SCORE];
        
      // Attributes - Total Reads 
      vector<string> covs = split( ev[TOTAL_COV], "/" );
      uint32_t cov = from_string<uint32_t>(covs[0]) + from_string<uint32_t>(covs[1]);
      gvf[8] = gvf[8].append(";Total_reads=").append(to_string(cov));
      
      // Attributes - Variant Reads
      vector<string> variant_covs = split( ev[NEW_COV], "/" );
      uint32_t variant_cov = from_string<uint32_t>(variant_covs[0]) + from_string<uint32_t>(variant_covs[1]);
      gvf[8] = gvf[8].append(";Variant_reads=").append(to_string(variant_cov));
        
      // Attributes - Frequency 
      gvf[8].append(";Variant_freq=").append( ev[FREQUENCY] );
      

      if (de.entry_exists("snp_type")) {
        if (de["snp_type"] == "nonsynonymous") {
          gvf[8].append(";Variant_effect=non_synonymous_codon");
        }
        else if (de["snp_type"] == "synonymous") {
          gvf[8].append(";Variant_effect=synonymous_codon");
        }
        else if (de["snp_type"] == "nonsense") {
          gvf[8].append(";Variant_effect=nonsense_codon");
        }
        else if (de["snp_type"] == "intergenic") {
          gvf[8].append(";Variant_effect=intergenic_variant");
        }
        else if (de["snp_type"] == "RNA") {
          gvf[8].append(";Variant_effect=nc_transcript_variant");
        }
        else if (de["snp_type"] == "pseudogene") {
          gvf[8].append(";Variant_effect=nc_transcript_variant");
        }
      }
    } // END of SNP
    
    else if( de._type == SUB ){
      gvf[2] = "indel";
      // End
      gvf[4] = to_string(from_string(de[POSITION]) + from_string(de[SIZE])); 
      gvf[8].append("Reference_seq=").append( ref_seq_info.get_sequence_1(de[SEQ_ID], from_string(de[POSITION]), from_string(de[POSITION]) + from_string(de[SIZE]) - 1));
      gvf[8].append(";Variant_seq=").append( de[NEW_SEQ] );
    }
    
    else if( de._type == DEL ){
      gvf[2] = "deletion";
      gvf[4] = gvf[3];
      gvf[8].append("Reference_seq=").append( ref_seq_info.get_sequence_1(de[SEQ_ID], from_string(de[POSITION]), from_string(de[POSITION]) + from_string(de[SIZE]) - 1) );
      gvf[8].append(";Variant_seq=").append( "." );
    }
    
    else if( de._type == INS ){
      gvf[2] = "insertion";
      gvf[4] = gvf[3];
      gvf[8].append("Reference_seq=").append( "." );
      gvf[8].append(";Variant_seq=").append( de[NEW_SEQ] );
    }
    
    else if( de._type == MOB ){
      gvf[2] = "mobile_element_insertion";
      gvf[4] = gvf[3];
      //Strand
      if( from_string(de["strand"]) > 0 )
        gvf[6] = "+";
      else
        gvf[6] = "-";
      gvf[8].append("Reference_seq=").append( "." );
      gvf[8].append(";Variant_seq=").append( mob_replace_sequence(ref_seq_info, de) );
    }
    
    else if( de._type == AMP )
    {
       gvf[2] = "copy_number_gain";
       stringstream ss;
       gvf[4] = gvf[3];
       gvf[8].append("Reference_seq=").append( "." );
       gvf[8].append(";Variant_seq=").append( ref_seq_info.get_sequence_1(de[SEQ_ID], from_string(de[POSITION]), from_string(de[POSITION]) + from_string(de[SIZE]) - 1) );
    }
    else if( de._type == INV ){
      gvf[2] = "inversion";
      gvf[4] = to_string(from_string(de[POSITION]) + from_string(de[SIZE]) - 1);
    }
    else if( de._type == CON ){
      gvf[2] = "substitution";
      gvf[4] = gvf[3];
      
      uint32_t tid, start_pos, end_pos;
      ref_seq_info.parse_region(de["region"], tid, start_pos, end_pos);
      
      gvf[8].append("Reference_seq=").append( ref_seq_info.get_sequence_1(de[SEQ_ID], from_string(de[POSITION]), from_string(de[POSITION]) + from_string(de[SIZE]) - 1) );
      gvf[8].append(";Variant_seq=").append( ref_seq_info.get_sequence_1(tid, start_pos, end_pos ));
    }
    
    // ID attribute
    if( gvf[8].compare( "" ) == 0 || ( gvf[8].size()>8 && (gvf[8].substr(0,3).compare("ID=") == 0)) ){
      string s = "";
      s.append("ID=").append(gvf[0]).append(":").append(gvf[1]).append(":");
      s.append(gvf[2]).append(":").append(gvf[3]).append(";");
      s.append(gvf[8]);
      gvf[8] = s;
    }
    
    if (!snv_only || (de._type == SNP))
      featuresGVF.push_back(gvf);
  }
  
  // Write results to file
  ofstream output( gvffile.c_str() );
  output << "##gff-version 3" << endl;
  output << "##gvf-version 1.0" << endl;
  output << "" << endl;
  output << "##source-method Source=breseq;Type=SNV;Dbxref=http://barricklab.org/breseq;Comment=Mapping and variant calling with breseq;" << endl;
  output << "" << endl;
  for( size_t i=0; i<featuresGVF.size(); i++ ){
    for( size_t j=0; j<featuresGVF[i].size(); j++ ){
      output << featuresGVF[i][j] << "\t";
    }
    output << "\n";
  }
  output.close();
  
}

  
void cGenomeDiff::read_vcf(const string &file_name)
{
  //VCF Column order.
  enum {CHROM = 0, POS, ID, REF, ALT, QUAL, FILTER, INFO};
  
  ifstream in(file_name.c_str());
  string line = "";
  
  while (!in.eof()) {
    getline(in, line);
    if (!in.good() || line.empty()) break;
    //Discard header lines for now.
    if (line[0] == '#') continue;
    const vector<string> &tokens = split(line, "\t");
    
    cDiffEntry de;
    const cString &rseq = tokens[REF], &aseq = tokens[ALT];
    if (aseq.contains(',')) {
      //! TO DO: Classify these mutations, GATK doesn't predict these(?)
      //        de._type = SUB;
      //        de[SEQ_ID]   = tokens[CHROM];
      //        de[POSITION] = tokens[POS];
      //        de[SIZE]     = to_string(tokens[ALT].size());
      //        de[NEW_SEQ]  = tokens[ALT];
      WARN("Can't classify line: " + line);
      continue;
    }
    if (rseq.size() > aseq.size()) {
      de._type = DEL;
      de[SEQ_ID]   = tokens[CHROM];
      de[POSITION] = to_string(from_string<uint32_t>(tokens[POS]) + aseq.size());
      de[SIZE]     = to_string(rseq.size() - aseq.size());
    }
    else if (rseq.size() < aseq.size()) {
      de._type = INS;
      de[SEQ_ID]   = tokens[CHROM];
      de[POSITION] = tokens[POS];
      de[NEW_SEQ]  = cString(aseq).remove_starting(rseq);
    }
    else if (rseq.size() == 1) {
      de._type = SNP;
      de[SEQ_ID]   = tokens[CHROM];
      de[POSITION] = tokens[POS];
      de[NEW_SEQ]  = aseq;
    }
    else {
      WARN("Can't classify line: " + line);
      continue;
    }
    
    const vector<string> &info_tokens = split(tokens[INFO], ";");
    //size_t n = info_tokens.size();
    for (size_t i = 0, n = info_tokens.size(); i < n; ++i) {
      cKeyValuePair kvp(info_tokens[i], '=');
      de[kvp.get_key()] = kvp.get_value();
    }
    
    add(de);
  }
  
}
 
// Creates a PHYLIP input file from a master list of mutations (from merged file), a list of genome diffs, and reference sequences. 
void cGenomeDiff::write_phylip(string& output_phylip_file_name, cGenomeDiff& master_gd, vector<cGenomeDiff>& gd_list, cReferenceSequences& ref_seq_info, bool missing_as_ancestral, bool verbose)
{
  (void) verbose;
  
  diff_entry_list_t mut_list = master_gd.mutation_list();
  
  ofstream out(output_phylip_file_name.c_str());
  out << gd_list.size() << " " << mut_list.size() << endl;
  
  for (vector<cGenomeDiff>::iterator gd_it = gd_list.begin(); gd_it != gd_list.end(); gd_it++) {
    
    string base_name = gd_it->get_title();
    const uint32_t phylip_name_max_length = 10;
    string base_name_truncated;
    if (base_name.size() > phylip_name_max_length)
      base_name_truncated = base_name.substr(0,phylip_name_max_length);
    else
      base_name_truncated = base_name + repeat_char(' ', phylip_name_max_length - base_name.size());
      
    out << base_name_truncated; 
    
    for (diff_entry_list_t::iterator it=mut_list.begin(); it != mut_list.end(); it++) {
      cDiffEntry& mut = **it;
      string key = "frequency_" + base_name;
      ASSERT(mut.count(key), "Did not find expected key: " + key + "\nIn item:\n" + mut.as_string());
      string val = mut[key];
      
      if (mut._type == SNP) {
        if ((val == "?") || (val == "D")) {
          if (missing_as_ancestral) {
            uint32_t position_1 = from_string<uint32_t>(mut[POSITION]);
            out << ref_seq_info.get_sequence_1(mut[SEQ_ID], position_1, position_1);
          } else {
            out << "N";
          }
        } else {
          //ASSERT(is_double(val), )
          double freq = from_string<double>(val);
          if (freq == 0.0) {        
            uint32_t position_1 = from_string<uint32_t>(mut[POSITION]);
            out << ref_seq_info.get_sequence_1(mut[SEQ_ID], position_1, position_1);
          }
          else if (freq == 1.0) out << mut[NEW_SEQ];
          else out << "N";
        }        
      } else {
        if ((val == "?") || (val == "D")) {
          if (missing_as_ancestral) {
            out << "A";
          } else {
            out << "N";
          }
        } else {
          //ASSERT(is_double(val), )
          double freq = from_string<double>(val);
          if (freq == 0.0) out << "A";
          else if (freq == 1.0) out << "T";
          else out << "N";
        }
      }
    }
    
    out << endl;
  }
}
    
 
// Unlike other conversion functions, takes a list of gd files
void cGenomeDiff::GD2Circos(const vector<string> &gd_file_names, 
                            const vector<string> &reference_file_names,
                            const string &circos_directory,
                            double distance_scale,
                            double feature_scale){
  
  cGenomeDiff combined_gd;
  
  string program_data_path = Settings::get_program_data_path();
  
  int32_t number_of_mutations = 0;
  
  for (size_t i = 0; i < gd_file_names.size(); i++){
    cGenomeDiff single_gd(gd_file_names[i]);
    combined_gd.merge(single_gd, false);
    number_of_mutations += single_gd.mutation_list().size();
  }
  
  cReferenceSequences ref;
  ref.LoadFiles(reference_file_names);
  ref.annotate_mutations(combined_gd, true);
  const vector<string> seq_ids(ref.seq_ids());
  
  string make_me;
  
  create_path(circos_directory);
  create_path(circos_directory + "/data");
  create_path(circos_directory + "/etc");
  
  //copy run script
  copy_file(program_data_path + "/run_circos.sh", circos_directory + "/run_circos.sh");
  
  //filling circos_dir/etc
  
  vector<string> conf_names = make_vector<string>
  ("indels.conf")
  ("mobs.conf")
  ("mutations.conf")
  ("combined_circos.conf")
  ;
  
  for (size_t i = 0; i < conf_names.size(); i++){
    copy_file(program_data_path + "/" + conf_names[i], circos_directory + "/etc/" + conf_names[i]);
  }
  
  //modifying circos_dir/etc with scale values
  double distance_value = 0.4 * distance_scale;
  double space_value = 0.25 * distance_value;
  double feature_value = 5 * feature_scale;
  
  map<string, string> replacement_map = make_map<string, string>
  (" = inner_distance_value_1", " = " + to_string(distance_value, 10) + "r")
  (" = inner_distance_value_2", " = " + to_string(distance_value + .01, 10) + "r")
  (" = inner_ticks", " = " + to_string(distance_value, 10) + "r")
  (" = syn_axis_value_1", " = " + to_string(distance_value + (2 * space_value), 10) + "r")
  (" = syn_axis_value_2", " = " + to_string(distance_value + (2 * space_value) + .01, 10) + "r")
  (" = nonsyn_axis_value_1", " = " + to_string(distance_value + (3 * space_value), 10) + "r")
  (" = nonsyn_axis_value_2", " = " + to_string(distance_value + (3 * space_value) + .01, 10) + "r")
  (" = npi_axis_value_1", " = " + to_string(distance_value + (4 * space_value), 10) + "r")
  (" = npi_axis_value_2", " = " + to_string(distance_value + (4 * space_value) + .01, 10) + "r")
  (" = mob_1_axis_value_1", " = " + to_string(distance_value + (5 * space_value), 10) + "r")
  (" = mob_1_axis_value_2", " = " + to_string(distance_value + (5 * space_value) + .01, 10) + "r")
  (" = mob_2_axis_value_1", " = " + to_string(distance_value + (6 * space_value), 10) + "r")
  (" = mob_2_axis_value_2", " = " + to_string(distance_value + (6 * space_value) + .01, 10) + "r")
  (" = outer_axis_value_1", " = " + to_string(distance_value + (7 * space_value), 10) + "r")
  (" = outer_axis_value_2", " = " + to_string(distance_value + (7 * space_value) + .01, 10) + "r")
  (" = outer_ticks", " = " + to_string(distance_value + (7 * space_value), 10) + "r")
  (" = indel_distance", " = " + to_string(distance_value + (7 * space_value) + .01, 10) + "r")
  (" = mob_distance", " = " + to_string(distance_value + (5.5 * space_value), 10) + "r - " + to_string(feature_value * 2.5) + "p")
  //(" = mob_distance", " = " + to_string(distance_value + (5 * space_value), 10) + "r + " + to_string((400 * .7 * space_value * .5) - (feature_value * 2.5), 10) + "p")
  (" = syn_distance", " = " + to_string(distance_value + (2 * space_value), 10) + "r - " + to_string(feature_value * 1.5, 10) + "p")
  (" = nonsyn_distance", " = " + to_string(distance_value + (3 * space_value), 10) + "r - " + to_string(feature_value * 1.5, 10) + "p")
  (" = npi_distance", " = " + to_string(distance_value + (4 * space_value), 10) + "r - " + to_string(feature_value * 1.5, 10) + "p")
  (" = indel_value", " = " + to_string(feature_value * 4, 10) + "p")
  (" = mob_value", " = " + to_string(feature_value * 5, 10) + "p")
  (" = snp_value", " = " + to_string(feature_value * 3, 10) + "p")
  (" = ind_syn_axis_value_1", " = " + to_string(1 + (1 * space_value), 10) + "r")
  (" = ind_syn_axis_value_2", " = " + to_string(1 + (1 * space_value) + .01, 10) + "r")
  (" = ind_nonsyn_axis_value_1", " = " + to_string(1 + (2 * space_value), 10) + "r")
  (" = ind_nonsyn_axis_value_2", " = " + to_string(1 + (2 * space_value) + .01, 10) + "r")
  (" = ind_npi_axis_value_1", " = " + to_string(1 + (3 * space_value), 10) + "r")
  (" = ind_npi_axis_value_2", " = " + to_string(1 + (3 * space_value) + .01, 10) + "r")
  (" = ind_syn_distance", " = " + to_string(1 + (1 * space_value), 10) + "r - " + to_string(feature_value * 1.5, 10) + "p")
  (" = ind_nonsyn_distance", " = " + to_string(1 + (2 * space_value), 10) + "r - " + to_string(feature_value * 1.5, 10) + "p")
  (" = ind_npi_distance", " = " + to_string(1 + (3 * space_value), 10) + "r - " + to_string(feature_value * 1.5, 10) + "p")
  (" = space_value_in_pixels", " = " + to_string((300 * space_value), 10) + "p")
  (" = ind_scale", " = " + to_string((.7 * distance_scale), 10) + "r")
  (" = label_size_value", " = " + to_string((16 * distance_scale), 10) + "p")
  (" = label_offset_value", " = " + to_string((-7.5 * distance_scale), 10) + "r")
  ;
  
  for (size_t i = 0; i < conf_names.size(); i++){
    replace_file_contents_using_map(circos_directory + "/etc/" + conf_names[i],
                                    circos_directory + "/etc/circos_temp.conf",
                                    replacement_map);
    copy_file(circos_directory + "/etc/circos_temp.conf",
              circos_directory + "/etc/" + conf_names[i]);
  }
  
  //filling circos_dir/data
  
  ofstream karyotype_file;
  ofstream empty_plot_file;
  
  make_me = circos_directory + "/data/karyotype.txt";
  karyotype_file.open(make_me.c_str());
  make_me = circos_directory + "/data/empty_data.txt";
  empty_plot_file.open(make_me.c_str());
  
  //keeps track of current position when examining sequence sizes of genomes
  int32_t current_position = 0;
  
  int32_t half_ref_length;
  half_ref_length = int32_t(ref.total_length() / 2) + 1;
  
  for (uint32_t i = 0; i < seq_ids.size(); i++){
    uint32_t seq_size;
    seq_size = ref[seq_ids[i]].get_sequence_size();
    
    karyotype_file << "chr - " << seq_ids[i] << " 1 1 " <<
    seq_size << " black" << endl;
    empty_plot_file << seq_ids[i] << " 1 2 1" << endl;
    
    //if seq_size goes past halfway point of total length of genomes,
    //add this sequence and its bounds to the left_side vector.
    current_position += seq_size;
  }
  
  karyotype_file.close();
  empty_plot_file.close();
  
  //minimum tile size width for indel graph
  const int32_t MIN_WIDTH = static_cast<int32_t>(floor(static_cast<double>(ref.total_length()) * 0.000));
  const int32_t MIN_DISPLAY_LENGTH = 51; //inclusive
  
  ofstream indel_file;
  ofstream mob_file;
  
  ofstream synonymous_mutation_file;
  ofstream nonsynonymous_mutation_file;
  ofstream npi_mutation_file;
  
  make_me = circos_directory + "/data/indels_data.txt";
  indel_file.open(make_me.c_str());
  make_me = circos_directory + "/data/mobs_data.txt";
  mob_file.open(make_me.c_str());
  
  make_me = circos_directory + "/data/syn_data.txt";
  synonymous_mutation_file.open(make_me.c_str());
  make_me = circos_directory + "/data/nonsyn_data.txt";
  nonsynonymous_mutation_file.open(make_me.c_str());
  make_me = circos_directory + "/data/npi_data.txt";
  npi_mutation_file.open(make_me.c_str());
  
  map <string, string> mob_colors;
  
  //colors for mobs
  const char* c_colors[] = {"vvdred", "vvdgreen", "vvdblue", "vvdorange", "vvdpurple",
    "dred", "dgreen", "dblue", "dorange", "dpurple",
    "red", "green", "blue", "orange", "purple",
    "lred", "lgreen", "lblue", "lorange", "lpurple",
    "vvlred", "vvlgreen", "vvlblue",  "vvlorange", "vvlpurple"};
  
  map<string,bool> pre_assigned_mob_colors = make_map<string,bool>
  ("is1"  , true )("is186", true )("is3"  , true  )
  ("is150", true )("is911", true )("is4"  , true  )
  ("is2"  , true )("is30" , true )("is600", true  )
  ;
  
  vector<string> colors(c_colors, c_colors + 25);
  string color;
  int32_t next_color = 0;
  
  //reference sequence MOBs
  for(size_t i = 0; i < ref.size(); i++){
    cAnnotatedSequence& ref_seq = ref[i];
    
    for(cFeatureLocationList::iterator it = ref_seq.m_repeat_locations.begin(); it != ref_seq.m_repeat_locations.end(); it++){
      cFeatureLocation& repeat = *it;
      cSequenceFeature& feature = *repeat.get_feature();
      int32_t middle = int32_t(repeat.get_start_1() + repeat.get_end_1()) / 2;
      
      string color;
      
      // Color assignment -- prefer preassigned, then grab next from list
      // and assign that color permanently to copies of this repeat
      if (pre_assigned_mob_colors.count(feature["name"])){
        color = feature["name"];
        mob_colors[feature["name"]] = color;
      }
      else if (mob_colors.count(feature["name"]) == 0){
        if (next_color >=25 ) next_color = 0; // this is how many colors are available above!
        color = colors[next_color];
        mob_colors[feature["name"]] = color;
        next_color++;
      }
      else{
        color = mob_colors[feature["name"]];
      }
      
      mob_file << ref_seq.m_seq_id << " " <<
      middle << " " <<
      middle << " " <<
      "i" << ((repeat.get_strand() == 1)? "right" : "left" ) << " " <<
      "color=" << color << endl;
    }
  }
  
  
  
  diff_entry_list_t gd_data = combined_gd.mutation_list();
  
  for (diff_entry_list_t::iterator it = gd_data.begin(); it != gd_data.end(); it++){
    
    cDiffEntry diff = **it;
    
    int32_t width;
    string direction;
    
    
    if (diff._type == INS){
      width = diff["new_seq"].size();
      if (width < MIN_DISPLAY_LENGTH){
        continue;
      }
      if (width < MIN_WIDTH){
        width = MIN_WIDTH;
      }
      
      indel_file << diff["seq_id"] << " " <<
      diff["position"] << " " <<
      from_string<int32_t>(diff["position"]) + width << " " <<
      "color=green" << endl;
    }
    else if (diff._type == AMP){
      width = from_string<int32_t>(diff["size"]);
      if (width < MIN_DISPLAY_LENGTH){
        continue;
      }
      if (width < MIN_WIDTH){
        width = MIN_WIDTH;
      }
      
      indel_file << diff["seq_id"] << " " <<
      diff["position"] << " " <<
      from_string<int32_t>(diff["position"]) + width << " " <<
      "color=green" << endl;
    }
    else if (diff._type == DEL){
      width = from_string<int32_t>(diff["size"]);
      if (width < MIN_DISPLAY_LENGTH){
        continue;
      }
      if (width < MIN_WIDTH){
        width = MIN_WIDTH;
      }
      indel_file << diff["seq_id"] << " " <<
      diff["position"] << " " <<
      from_string<int32_t>(diff["position"]) + width << " " <<
      "color=red" << endl;
      
      // Show new IS for IS mediated deletions
      if (diff.count("mediated")) {
        
        if (mob_colors.count(diff["mediated"])) { 
          // either the end or the beginning is in an IS element
          
          int32_t max_distance_to_repeat_1 = 0;
          int32_t max_distance_to_repeat_2 = 0;
          cFeatureLocation* feat1 = cReferenceSequences::find_closest_repeat_region_boundary(n(diff["position"]) - 1, ref[diff["seq_id"]].m_repeats, max_distance_to_repeat_1,-1);
          cFeatureLocation* feat2 = cReferenceSequences::find_closest_repeat_region_boundary(n(diff["position"]) + n(diff["size"]) + 1 - 1, ref[diff["seq_id"]].m_repeats, max_distance_to_repeat_2,1);
          
          if (!feat1 && !feat2) {
            cerr << diff << endl;
            ASSERT(false,"Could not find mediating repeat.");
          }  
          
          cFeatureLocation& repeat = feat1 ? *feat1: *feat2;
          cSequenceFeature& feature = *(repeat.get_feature());
          cAnnotatedSequence& ref_seq = ref[diff["seq_id"]];
          
          int32_t middle = feat1 ? n(diff["position"]) + n(diff["size"]) - 1 : n(diff["position"]);
          
          string color;
          
          // Color assignment -- prefer preassigned, then grab next from list
          // and assign that color permanently to copies of this repeat
          if (pre_assigned_mob_colors.count(feature["name"])){
            color = feature["name"];
          }
          else if (mob_colors.count(feature["name"]) == 0){
            color = colors[next_color];
            mob_colors[feature["name"]] = color;
            next_color++;
          }
          else{
            color = mob_colors[feature["name"]];
          }
          
          mob_file << ref_seq.m_seq_id << " " <<
          middle << " " <<
          middle << " " <<
          "o" << ((repeat.get_strand() == 1)? "right" : "left" ) << " " <<
          "color=" << color << endl;
        }
      }
    }
    else if(diff._type == SNP || diff._type == SUB){
      if (diff["snp_type"] == "synonymous"){
        synonymous_mutation_file << diff["seq_id"] << " " <<
        diff["position"] << " " <<
        diff["position"] << endl;
      }
      // Should split out nonsense @JEB
      else if ((diff["snp_type"] == "nonsynonymous") || (diff["snp_type"] == "nonsense")){
        nonsynonymous_mutation_file << diff["seq_id"] << " " <<
        diff["position"] << " " <<
        diff["position"] << endl;
      }
      else{
        npi_mutation_file << diff["seq_id"] << " " <<
        diff["position"] << " " <<
        diff["position"] << endl;
      }
      
    }
    
    else if(diff._type == MOB){
      
      // Color assignment -- prefer preassigned, then grab next from list
      // and assign that color permanently to copies of this repeat
      if (pre_assigned_mob_colors.count(diff["repeat_name"])){
        color = diff["repeat_name"];
      }
      else if (mob_colors.count(diff["repeat_name"]) == 0){
        color = colors[next_color];
        mob_colors[diff["repeat_name"]] = color;
        next_color++;
      }
      else{
        color = mob_colors[diff["repeat_name"]];
      }
      
      mob_file << diff["seq_id"] << " " <<
      diff["position"] << " " <<
      diff["position"] << " " <<
      "o" << ((n(diff["strand"]) == 1)? "right" : "left" ) << " " <<
      "color=" << color << endl;
    }
  }
  
  
  indel_file.close();
  mob_file.close();
  synonymous_mutation_file.close();
  nonsynonymous_mutation_file.close();
  npi_mutation_file.close();
  

  SYSTEM("cp " + Settings::get_program_data_path() + "/run_circos.sh " + circos_directory);
  
} 
  
/*
Temporary format for exchange with Olivier Tenaillon to analyze parallelism
 
 Example lines
 
 Ancestor	0	B_REL606	3040	880528	Duplication_2_fold	24169_bp	Multigenic	888	ECB_00814	Multigenic	926	ECB_misc_RNA_22	12	Ara+6_500_B_Ara+6_773A	Ara+5_1500_B_Ara+5_1066B	Ara+5_1000_B_Ara+5_962A	Ara+2_500_B_Ara+2_769B	Ara+2_500_B_Ara+2_769A	Ara+1_2000_B_Ara+1_1158A	Ara+1_1000_B_Ara+1_958A	Ara-2_2000_B_Ara-2_1165A	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_2000_B_Ara-1_1164B	Ancestor_0_B_REL606	
 Ara-1	2000	B_Ara-1_1164B	3040	880528	Duplication_2_fold	24169_bp	Multigenic	888	ECB_00814	Multigenic	926	ECB_misc_RNA_22	12	Ara+6_500_B_Ara+6_773A	Ara+5_1500_B_Ara+5_1066B	Ara+5_1000_B_Ara+5_962A	Ara+2_500_B_Ara+2_769B	Ara+2_500_B_Ara+2_769A	Ara+1_2000_B_Ara+1_1158A	Ara+1_1000_B_Ara+1_958A	Ara-2_2000_B_Ara-2_1165A	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_2000_B_Ara-1_1164B	Ancestor_0_B_REL606	
 Ara-1	2000	B_Ara-1_1164B	4572	1329516	Mutation	C->T	Gene	1356	topA	NonSynonymous H33Y	33	865	23	Ara-1_50000_Ara-1_11330A	DVS3S5_0_DVS3S5	Ara-1_50000_11331	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_1000_964B	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_27000_B_Ara-1_10273	Ara-1_5000_B_Ara-1_2179B	Ara-1_15000_B_Ara-1_7177C	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164B	4970	1435468	Mutation	C->T	Gene	1470	ydbH	NonSynonymous A271V	271	879	1	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164B	5978	1733582	Insertion	7_bp	Gene	1777	pykF	Frameshift	206	470	1	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164B	12964	3762741	Mutation	A->T	Gene	3824	spoT	NonSynonymous K662I	662	702	23	Ara-1_50000_Ara-1_11330A	DVS3S5_0_DVS3S5	Ara-1_50000_11331	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_1000_964B	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_27000_B_Ara-1_10273	Ara-1_5000_B_Ara-1_2179B	Ara-1_15000_B_Ara-1_7177C	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164B	13303	3875627	Insertion	1_bp	Intergenic	3941	glmU(61)	Intergenic	atpC(292)	Intergenic_1_bp_Insertion	23	Ara-1_1000_964B	Ara-1_50000_11331	Ara-1_1500_1068C	Ara-1_1500_1068B	DVS3S5_0_DVS3S5	Ara-1_50000_Ara-1_11330A	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_27000_B_Ara-1_10273	Ara-1_5000_B_Ara-1_2179B	Ara-1_15000_B_Ara-1_7177C	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164B	13398	3895000	LargeDeletion	6923_bp	Multigenic	3963	rbsD	Multigenic	3969	hsrA	6	Ara-1_50000_11331	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_1000_964B	
 Ara-1	2000	B_Ara-1_1164B	13443	3901929	IS_Insertion	IS150	Gene	3969	hsrA	IS_insertion	-164	475	23	DVS3S5_0_DVS3S5	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_15000_B_Ara-1_7177C	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_5000_B_Ara-1_2179B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_27000_B_Ara-1_10273	Ara-1_50000_Ara-1_11330A	Ara-1_1000_964B	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_50000_11331	
 Ara-1	2000	B_Ara-1_1164C	3040	880528	Duplication_2_fold	24169_bp	Multigenic	888	ECB_00814	Multigenic	926	ECB_misc_RNA_22	12	Ara+6_500_B_Ara+6_773A	Ara+5_1500_B_Ara+5_1066B	Ara+5_1000_B_Ara+5_962A	Ara+2_500_B_Ara+2_769B	Ara+2_500_B_Ara+2_769A	Ara+1_2000_B_Ara+1_1158A	Ara+1_1000_B_Ara+1_958A	Ara-2_2000_B_Ara-2_1165A	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_2000_B_Ara-1_1164B	Ancestor_0_B_REL606	
 Ara-1	2000	B_Ara-1_1164C	4572	1329516	Mutation	C->T	Gene	1356	topA	NonSynonymous H33Y	33	865	23	Ara-1_50000_Ara-1_11330A	DVS3S5_0_DVS3S5	Ara-1_50000_11331	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_1000_964B	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_27000_B_Ara-1_10273	Ara-1_5000_B_Ara-1_2179B	Ara-1_15000_B_Ara-1_7177C	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164C	5965	1733101	Mutation	G->C	Gene	1777	pykF	NonSynonymous R46P	46	470	1	Ara-1_2000_B_Ara-1_1164C	
 Ara-1	2000	B_Ara-1_1164C	6098	1757661	IS_Insertion	IS150	Intergenic	1801	ydiQ(12)	Intergenic	ydiR(8)	Intergenic_IS_insertion	1	Ara-1_2000_B_Ara-1_1164C	
 Ara-1	2000	B_Ara-1_1164C	12964	3762741	Mutation	A->T	Gene	3824	spoT	NonSynonymous K662I	662	702	23	Ara-1_50000_Ara-1_11330A	DVS3S5_0_DVS3S5	Ara-1_50000_11331	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_1000_964B	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_27000_B_Ara-1_10273	Ara-1_5000_B_Ara-1_2179B	Ara-1_15000_B_Ara-1_7177C	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164C	13303	3875627	Insertion	1_bp	Intergenic	3941	glmU(61)	Intergenic	atpC(292)	Intergenic_1_bp_Insertion	23	Ara-1_1000_964B	Ara-1_50000_11331	Ara-1_1500_1068C	Ara-1_1500_1068B	DVS3S5_0_DVS3S5	Ara-1_50000_Ara-1_11330A	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_27000_B_Ara-1_10273	Ara-1_5000_B_Ara-1_2179B	Ara-1_15000_B_Ara-1_7177C	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	
 Ara-1	2000	B_Ara-1_1164C	13398	3895000	LargeDeletion	6923_bp	Multigenic	3963	rbsD	Multigenic	3969	hsrA	6	Ara-1_50000_11331	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_1000_964B	
 Ara-1	2000	B_Ara-1_1164C	13443	3901929	IS_Insertion	IS150	Gene	3969	hsrA	IS_insertion	-164	475	23	DVS3S5_0_DVS3S5	Ara-1_20000_B_Ara-1_8593C	Ara-1_20000_B_Ara-1_8593B	Ara-1_15000_B_Ara-1_7177C	Ara-1_15000_B_Ara-1_7177B	Ara-1_10000_B_Ara-1_4536C	Ara-1_10000_B_Ara-1_4536B	Ara-1_5000_B_Ara-1_2179C	Ara-1_5000_B_Ara-1_2179B	Ara-1_2000_B_Ara-1_1164C	Ara-1_2000_B_Ara-1_1164B	Ara-1_40000_B_Ara-1_10940	Ara-1_40000_B_Ara-1_10939	Ara-1_40000_B_Ara-1_10938	Ara-1_35000_B_Ara-1_10707	Ara-1_30000_B_Ara-1_10392	Ara-1_30000_B_Ara-1_10391	Ara-1_27000_B_Ara-1_10273	Ara-1_50000_Ara-1_11330A	Ara-1_1000_964B	Ara-1_1500_1068C	Ara-1_1500_1068B	Ara-1_50000_11331	
*/
  
  
void cGenomeDiff::GD2OLI(const vector<string> &gd_file_names, 
                         const vector<string> &reference_file_names, 
                         const string& output_file_name,
                         const uint32_t large_size_cutoff,
                         const bool phylogeny_aware)
{
  cReferenceSequences ref_seq_info;
  ref_seq_info.LoadFiles(reference_file_names);
  
  ofstream out(output_file_name.c_str(), ios::out);
  
  // Create merged master list of mutations to annotate.
  cout << "Loading/Merging Genome Diff files..." << endl;

  cGenomeDiff master_gd;
  vector<cGenomeDiff> gd_list;
  for(vector<string>::const_iterator it=gd_file_names.begin(); it != gd_file_names.end(); it++) {
    cout << "   " << *it << endl;
    
    cGenomeDiff this_gd(*it);
    gd_list.push_back(this_gd);
    
    master_gd.merge(this_gd, true, false, phylogeny_aware); // last true is to make phylogeny aware within populations
  }
  
  cGenomeDiff::sort_gd_list_by_treatment_population_time(gd_list);
  
  
  // Annotate all the mutations once
  cout << "Annotating mutations in merged file..." << endl;
  ref_seq_info.annotate_mutations(master_gd, true, false);
  
  // Add frequency columns
  vector<string> title_list;
  cGenomeDiff::tabulate_frequencies_from_multiple_gds(master_gd, gd_list, title_list, false);

  vector<string> header_list(14, "");
  header_list[0] = "pop";
  header_list[1] = "age";
  header_list[2] = "run";
  header_list[3] = "event_number";
  header_list[4] = "position";
  header_list[5] = "Type";
  header_list[6] = "Change";
  header_list[13] = "nbtrains_affected";

  header_list[7] = "Intergenic";
  header_list[8] = "Previous_Gene_nb";
  header_list[9] = "Previous_Gene_Name(distance_bp)";
  header_list[10] = "Effect";
  header_list[11] = "Next_Gene_Name(distance_bp)";
  header_list[12] = "Intergenic_type";
  out << join(header_list, "\t") << endl;
 
  header_list[7] = "Multigenic";
  header_list[8] = "First_Gene_nb";
  header_list[9] = "First_Gene_Name";
  header_list[10] = "Effect";
  header_list[11] = "Last_Gene_nb";
  header_list[12] = "Last_Gene_Name";
  out << join(header_list, "\t") << endl;
  
  header_list[7] = "Gene";
  header_list[8] = "Gene_nb";
  header_list[9] = "Gene_Name";
  header_list[10] = "Large_Deletion";
  header_list[11] = "bp_deleted_in_Gene";
  header_list[12] = "gene_length_bp";
  out << join(header_list, "\t") << endl;
  
  // For each gd file, march through the master list and output anything with
  // a frequency of "1".
  diff_entry_list_t mut_list = master_gd.mutation_list();
  
  string pop;
  string age;
  string run;

  size_t title_counter = 0;
  for(vector<cGenomeDiff>::iterator it = gd_list.begin(); it != gd_list.end(); it++) {
    uint32_t event_num = 0;
    
    pop = it->metadata.population;
    age = to_string<double>(it->metadata.time);
    run = title_list[title_counter++];
    
    for(diff_entry_list_t::iterator mut_it = mut_list.begin(); mut_it != mut_list.end(); mut_it++) {
      event_num++;
      string key( "frequency_" + it->get_title() );
      
      cDiffEntry  mut = **mut_it;
      if (!mut.entry_exists(key)) continue;
      if (mut[key] != "1") continue;
      
      
      ///////// VARIABLES to FILL IN //////////
      string position;
      string type;
      string change;
      string genic;         // Gene, Intergenic, Multigenic
      
      vector<string> gene_columns;
      // Three different situations for these columns
      // Intergenic
      // --> Previous_Gene_nb, Previous_Gene_Name(distance_bp), Effect, Next_Gene_Name(distance_bp, Intergenic_type
      // Multigenic
      // --> First_Gene_nb	First_Gene_Name	Effect	Last_Gene_nb	Last_Gene_Name
      // Gene	
      // -->Gene_nb	Gene_Name	Large_Deletion	bp_deleted_in_Gene	gene_length_bp	
      
      // common attributes for any mutation
      position = mut[POSITION];
      
      if (mut._type == SNP) {
        type = "Mutation"; 
        
        uint32_t position_1 = from_string<uint32_t>(mut[POSITION]);
        change = ref_seq_info.get_sequence_1(mut[SEQ_ID], position_1, position_1) + "->" + mut[NEW_SEQ];
      }
      
      else if (mut._type == DEL) {
        uint32_t size = from_string<uint32_t>(mut[SIZE]);
        if (size > large_size_cutoff)
          type = "LargeDeletion";
        else
          type = "Deletion";
        change = mut[SIZE] + "_bp";
      }
      
      else if (mut._type == INS) {
        type = "Insertion";
        change = to_string<uint32_t>(mut[NEW_SEQ].size()) + "_bp";
      }
      
      else if (mut._type == SUB) {
        int32_t old_size = from_string<int32_t>(mut[SIZE]);
        int32_t new_size = mut[NEW_SEQ].size();
        
        // This is more of a big deletion. Erg.
        if (old_size - new_size > static_cast<int32_t>(large_size_cutoff)) {
          type="LargeDeletion";
          change = to_string<int32_t>(old_size - new_size) + "_bp";
        }
        else {
          type="Substitution";
          change = to_string<int32_t>(old_size) + "->" + to_string<int32_t>(new_size) + "_bp";
        }
      }
      
      // skip gene conversion
      else if (mut._type == CON) {
        continue;
      }
      
      // skip inversion
      else if (mut._type == INV) {
        continue;
      }
      
      else if (mut._type == MOB) {
        type = "IS_Insertion";
        change = mut["repeat_name"];        
      } 
      
      else if (mut._type == AMP) {
        
        uint32_t size = from_string<uint32_t>(mut[SIZE]) * (from_string<uint32_t>(mut["new_copy_number"]) - 1);
        if (size > large_size_cutoff) {
          type = "Duplication_" + mut["new_copy_number"] + "_fold";
          change = mut[SIZE] + "_bp";
        }
        else {
          type = "Insertion";
          change = to_string(size) + "_bp";
        }
      }
      
      else {
        ERROR("Mutation type \"" + to_string(mut._type) + "\" is not handled!");
      }
      
      // Fill in gene information
      
      vector<string> genic_columns;
      genic_columns.push_back(".");
      genic_columns.push_back(".");
      genic_columns.push_back(".");
      genic_columns.push_back(".");
      genic_columns.push_back(".");

      
      if (mut["gene_name"].find("/") != string::npos) {
        genic = "Intergenic";
        
        /*
        genic_columns.push_back("0"); //Previous_Gene_nb ?????
        // gene_position looks like this "intergenic (+180/–)"
        size_t pos = 0;
        string s = mut["gene_position"];
        pos_start = s.find_first_of("0123456789", pos);
        pos_end = s.find_first_not_of("0123456789", pos);
        genic_columns.push_back("0");
        
        genic_columns.push_back("0");
        
        vector<string> split_line = find( "/");
                                      //Previous_Gene_Name(distance_bp)
        
        (-201/+89)
        */
      } else if (mut["gene_name"].find(",") != string::npos) {
        genic = "Multigene";
      } else {
        genic = "Gene";
      }
      
      // Intergenic
      // --> Previous_Gene_nb, Previous_Gene_Name(distance_bp), Effect, Next_Gene_Name(distance_bp, Intergenic_type
      // Multigenic
      // --> First_Gene_nb	First_Gene_Name	Effect	Last_Gene_nb	Last_Gene_Name
      // Gene	
      // -->Gene_nb	Gene_Name	Large_Deletion	bp_deleted_in_Gene	gene_length_bp	
    
       
      
      // Output the finished line
      vector<string> line_list;
      line_list.push_back(pop);
      line_list.push_back(age);
      line_list.push_back(run);
      line_list.push_back(to_string(event_num));
      line_list.push_back(position);
      line_list.push_back(type);
      line_list.push_back(change);
      line_list.push_back(genic);
      line_list.insert(line_list.end(), genic_columns.begin(), genic_columns.end());

      // Add all of the other samples with this
      for(vector<cGenomeDiff>::iterator it3 = gd_list.begin(); it3 != gd_list.end(); it3++) {
        string key( "frequency_" + it3->get_title() );
        if (!mut.entry_exists(key) || (mut[key] != "1")) continue;
        line_list.push_back(it3->get_title());
        //line_list.push_back(it3->metadata.population + "_" + formatted_double(it3->metadata.time,0) .to_string() + "_" + it3->metadata.clone);
      }
      
      out << join(line_list, "\t") << endl;
      
    }
  }
  
}
  

// For creating coverage graphs in R
void cGenomeDiff::GD2COV(const vector<string> &gd_file_names,
                         const vector<string> &reference_file_names,
                         const string& output_file_name,
                         const uint32_t chunk_size)
{
  cReferenceSequences ref_seq_info;
  ref_seq_info.LoadFiles(reference_file_names);
  
  //assumes single reference file for now, could output concatenation
  uint32_t ref_seq_length = ref_seq_info[0].get_sequence_length();
  
  cout << "Loading Genome Diff files..." << endl;
  
  vector<cGenomeDiff> gd_list;
  for(vector<string>::const_iterator it=gd_file_names.begin(); it != gd_file_names.end(); it++) {
    cout << "   " << *it << endl;
    cGenomeDiff this_gd(*it);
    gd_list.push_back(this_gd);
  }
  
  cGenomeDiff::sort_gd_list_by_treatment_population_time(gd_list);

  uint32_t num_chunk = trunc(ref_seq_length / double(chunk_size)) + 1;
  
  vector<string> headers;
  vector< vector<int32_t> > dup_cov;
  vector< vector<int32_t> > del_cov;
  
  for(vector<cGenomeDiff>::iterator it = gd_list.begin(); it != gd_list.end(); it++) {
    
    cGenomeDiff& gd = *it;
    
    headers.push_back(gd.get_title());
    vector<int32_t> this_dup_cov(num_chunk, 0);
    vector<int32_t> this_del_cov(num_chunk, 0);
    
    diff_entry_list_t mut_list = gd.get_list(make_vector<gd_entry_type>(DEL)(AMP));

    for(diff_entry_list_t::iterator mut_it = mut_list.begin(); mut_it != mut_list.end(); mut_it++) {
      
      cDiffEntry& mut = **mut_it;
      
      if (from_string<int32_t>(mut[SIZE]) < 100) {
        continue;
      }
      
      if (mut._type == DEL) {
        
        int32_t start = trunc( mut.get_reference_coordinate_start().get_position() / double(chunk_size) ) ;
        int32_t end = trunc( mut.get_reference_coordinate_end().get_position() / double(chunk_size) );
        
        for (int32_t i = start; i <= end; i++) {
          this_del_cov[i] += 1;
        }
        
      } else if (mut._type == AMP) {
        
        int32_t start = trunc( mut.get_reference_coordinate_start().get_position() / double(chunk_size) ) ;
        int32_t end = trunc( mut.get_reference_coordinate_end().get_position() / double(chunk_size) );
        int32_t add_copy_num = from_string<int32_t>(mut[NEW_COPY_NUMBER]) - 1;
        
        for (int32_t i = start; i < end; i++) {
          this_dup_cov[i] += add_copy_num;
        }
      }
    }
    
    dup_cov.push_back(this_dup_cov);
    del_cov.push_back(this_del_cov);

  }
  
  ofstream delout( (output_file_name + ".del.tab").c_str() );
  ofstream dupout( (output_file_name + ".dup.tab").c_str() );
  
  delout << join( make_vector<string>("position")("title")("population")("time")("coverage"), "\t") << endl;
  dupout << join( make_vector<string>("position")("title")("population")("time")("coverage"), "\t") << endl;

  
  uint32_t j=0;
  for(vector<cGenomeDiff>::iterator it = gd_list.begin(); it != gd_list.end(); it++) {
    
    for (uint32_t i=0; i<num_chunk; i++) {
      
      delout << (i * chunk_size);
      delout << "\t" << it->get_title();
      delout << "\t" << it->metadata.population;
      delout << "\t" << it->metadata.time;
      delout << "\t" << del_cov[j][i];
      delout << endl;
      
      dupout << (i * chunk_size);
      dupout << "\t" << it->metadata.population;
      dupout << "\t" << it->metadata.time;
      dupout << "\t" << it->get_title();
      dupout << "\t" << dup_cov[j][i];
      dupout << endl;
      
      /* Alternative method that only plots changes
      if ((i==0) || (i==num_chunk-1) || (del_cov[j][i] != del_cov[j][i-1]) || (del_cov[j][i] != del_cov[j][i+1]))
      {
        delout << (i * chunk_size);
        delout << "\t" << it->get_title();
        delout << "\t" << del_cov[j][i];
        delout << endl;
      }
      
      if ((i==0) || (i==num_chunk-1) || (dup_cov[j][i] != dup_cov[j][i-1]) || (dup_cov[j][i] != dup_cov[j][i+1]))
      {
        dupout << (i * chunk_size);
        dupout << "\t" << it->get_title();
        dupout << "\t" << dup_cov[j][i];
        dupout << endl;
      }
      */
    }
    
    j++;
  }
  
  
}
  
bool sort_gd_by_treatment_population_time(const cGenomeDiff& a, const cGenomeDiff &b)
{
  // Treatment
  if (a.metadata.treatment != b.metadata.treatment)
    return (a.metadata.treatment < b.metadata.treatment);
  // Population
  if (a.metadata.population != b.metadata.population)
    return (a.metadata.population < b.metadata.population); 
  // Time
  if (a.metadata.time != b.metadata.time)
    return (a.metadata.time < b.metadata.time); 
  // Clone
  if (a.metadata.clone != b.metadata.clone)
    return (a.metadata.clone < b.metadata.clone);
  
  return (a.metadata.input_order < b.metadata.input_order);
}

void cGenomeDiff::sort_gd_list_by_treatment_population_time(vector<cGenomeDiff>& genome_diffs)
{
  std::sort(genome_diffs.begin(), genome_diffs.end(), sort_gd_by_treatment_population_time);
}

}//namespace bresesq


