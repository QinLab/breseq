/*****************************************************************************

AUTHORS

  Jeffrey E. Barrick <jeffrey.e.barrick@gmail.com>
  David B. Knoester

LICENSE AND COPYRIGHT

  Copyright (c) 2008-2010 Michigan State University
  Copyright (c) 2011 The University of Texas at Austin

  breseq is free software; you can redistribute it and/or modify it under the  
  terms the GNU General Public License as published by the Free Software 
  Foundation; either version 1, or (at your option) any later version.

*****************************************************************************/

#include "libbreseq/common.h"
#include "libbreseq/settings.h"
#include "libbreseq/anyoption.h"
#include "libbreseq/genome_diff.h"
#include "libbreseq/annotated_sequence.h"
#include "libbreseq/output.h"

using namespace breseq;
using namespace output;

int gdtools_usage()
{
  UserOutput uout(0);
  uout("Usage: gdtools [COMMAND] [OPTIONS]");
  uout("Manipulate Genome Diff (*.gd) files using the following commands.");

  uout("General:");
  uout << "apply                  apply mutations to a sequence" << endl;
  uout << "compare                compare control versus test mutations" << endl;
  uout << "not-evidence           remove evidence not used by any mutations" << endl;
  uout << "annotate               get additional mutation information from sequence" << endl;
  uout << "normalize              normalize mutations to a sequence" << endl;
  uout << "filter                 remove mutations given filtering expressions" << endl;
  uout << "merge                  combine multiple GD files" << endl;

  uout("Format Conversions:");
  uout << "gd2gvf                 GD to Genome Variant Format(GVF)" << endl;
  uout << "vcf2gd                 Variant Call Format(VCF) to GD" << endl;

  uout("Set Operations:");
  uout << "subtract               remove mutations" << endl;
  uout << "intersect              locate equal mutations" << endl;
  uout << "union                  combine mutations, removing duplicates" << endl;
  
  return 0;
}

int do_intersection(int argc, char *argv[])
{
  AnyOption options("INTERSECT -o <output.gd> <input1.gd input2.gd ...>");
  options("output,o",  "output GD file name");
  options("verbose,v", "verbose mode", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);

  options.addUsage("");
  options.addUsage("\tCreates a GD file of mutations and associated evidence that exists across all input GD files.");

  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }  

  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("No additional files provided.");
    options.printUsage();
    return -1;
  }

  if (options.getArgc() < 2) {
    options.addUsage("");
    options.addUsage("Not enough additional files provided.");
    options.printUsage();
    return -1;
  }

  UserOutput uout("INTERSECT");

  uout("Reading input GD files") << options.getArgv(0) << endl;
  cGenomeDiff gd1(options.getArgv(0));

  for(int32_t i = 1; i < options.getArgc(); ++i) {
    uout << options.getArgv(i) << endl;
    cGenomeDiff gd2(options.getArgv(i));
    gd1.set_intersect(gd2, options.count("verbose"));
  }

  uout("Writing output GD file", options["output"]);
  gd1.write(options["output"]);

  return 0;
}

int do_union(int argc, char *argv[])
{
  AnyOption options("UNION -o <output.gd> <input1.gd input2.gd ...>");
  options("output,o",  "output GD file name");
  options("verbose,v", "verbose mode", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);

  options.addUsage("");
  options.addUsage("\tCreates a GD file of mutations and associated evidence that exist in each input GD file.");
  options.addUsage("Duplicate mutations are merged into a single mutation.");

  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }  

  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("No additional files provided.");
    options.printUsage();
    return -1;
  }

  if (options.getArgc() < 2) {
    options.addUsage("");
    options.addUsage("Not enough additional files provided.");
    options.printUsage();
    return -1;
  }
  UserOutput uout("UNION");

  uout("Reading input GD files") << options.getArgv(0) << endl;
  cGenomeDiff gd1(options.getArgv(0));

  for(int32_t i = 1; i < options.getArgc(); ++i) {
    uout << options.getArgv(0) << endl;
    cGenomeDiff gd2(options.getArgv(i));
    gd1.set_union(gd2, options.count("verbose"));
  }

  uout("Assigning unique IDs");
  gd1.assign_unique_ids();

  uout("Writing output GD file", options["output"]); 
  gd1.write(options["output"]);

  return 0;
}

int do_mutate(int argc, char *argv[])
{
  AnyOption options("APPLY -o <output> -f <fasta/gff3> -r <reference> <input.gd>");
  options("output,o",    "output file");
  options("format,f",    "output file's format(Options: fasta, gff3)");
  options("reference,r", ".gbk/.gff3/.fasta/.bull reference sequence file");
  options("verbose,v",   "Verbose Mode (Flag)", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);
  
  options.addUsage("");
  options.addUsage("Input a single GenomeDiff, and as many reference files");
  options.addUsage("as you like.  Using the GenomeDiff, this will apply all");
  options.addUsage("the mutations to the reference sequences, output is to");
  options.addUsage("a single file that includes all the references.");
  
  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }  

  if (!options.count("format")) {
    options.addUsage("");
    options.addUsage("No format provided.");
    options.printUsage();
    return -1;
  } else {
    options["format"] = to_upper(options["format"]);
  }

  if (options["format"] != "FASTA" || options["format"] != "GFF3") {
    options.addUsage("");
    options.addUsage("No option for provided format.");
    options.printUsage();
    return -1;
  }

  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("No additional files provided.");
    options.printUsage();
    return -1;
  }

  UserOutput uout("APPLY");

  cReferenceSequences ref_seq_info;
  cReferenceSequences new_ref_seq_info;

  const vector<string>& ref_paths = from_string<vector<string> >(options["reference"]);
  uout("Reading input reference files", ref_paths);
  ref_seq_info.LoadFiles(ref_paths);
  new_ref_seq_info.LoadFiles(ref_paths);


  uout("Reading input GD file", options.getArgv(0));
  cGenomeDiff gd(options.getArgv(0));


  //Check to see if every item in the loaded .gd is applicable to the reference file.
  ASSERT(gd.is_valid(ref_seq_info, options.count("verbose")), "Reference file and GenomeDiff file don't match.");
  gd.apply_to_sequences(ref_seq_info, new_ref_seq_info, options.count("verbose"));

  uout("Writing output file in " + options["format"] + " format");

  if (options["format"] == "FASTA") {
    uout << options["fasta"] << endl;
    new_ref_seq_info.WriteFASTA(options["fasta"], options.count("verbose"));
  }
  else
  if (options["format"] == "GFF3") {
    uout << options["gff3"] << endl;
    new_ref_seq_info.WriteGFF(options["gff3"], options.count("verbose"));
  }

  return 0;
}

int do_subtract(int argc, char *argv[])
{
  AnyOption options("SUBTRACT [-o output.gd] input.gd minus1.gd [minus2.gd ...]");
  options("output,o",  "output GD file");
  options("verbose,v", "verbose mode", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);
  
  options.addUsage("");
  options.addUsage("Creates a GD file of mutations from the input.gd after removing mutations present in additional minus#.gd GD files.");
  
  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }

  if (options.getArgc() < 2) {
    options.addUsage("");
    options.addUsage("Not enough input files provided.");
    options.printUsage();
    return -1;
  }

  const bool verbose = options.count("verbose");

  UserOutput uout("SUBTRACT");

  uout("Starting with input GD file", options.getArgv(0));
  cGenomeDiff gd1(options.getArgv(0));

  uout("Subtracting mutations from files");
  for (int32_t i = 1; i < options.getArgc(); ++i) {
    uout << options.getArgv(i) << endl;
    cGenomeDiff gd2(options.getArgv(i));
    gd1.set_subtract(gd2, verbose);
  }
  
  uout("Writing output GD file", options["output"]);
  gd1.write(options["output"]);
  
  return 0;
}

int do_merge(int argc, char *argv[])
{
  AnyOption options("MERGE -o <output.gd> <input1.gd input2.gd ...>");
  options("output,o",     "output GD file");
  options("unique,u",     "unique entries only", TAKES_NO_ARGUMENT);  
  options("id,i",         "reorder IDs", TAKES_NO_ARGUMENT);
  options("verbose,v",    "verbose mode", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);
  
  options.addUsage("");
  options.addUsage("Input as many GenomeDiff files as you want, and have them");
  options.addUsage("merged together into a single GenomeDiff file specified.");
  options.addUsage("Unique IDs will remain unique across files, any IDs that");
  options.addUsage("aren't unique will get new ones.  Mutations that use those");
  options.addUsage("IDs will be properly updated to acknowledge the change.");
  
  if(argc == 1)  {
    options.printUsage();
    return -1;  }
  
  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("No input provided.");
    options.printUsage();
    return -1;
  }
  
  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("no output provided.");
    options.printUsage();
    return -1;
  }

  const bool verbose = options.count("verbose");

  UserOutput uout("MERGE");
  
  uout("Merging input GD files") << options.getArgv(0) << endl;
  cGenomeDiff gd1(options.getArgv(0));

  //Load all the GD files that were input.
  for(int32_t i = 1; i < options.getArgc(); i++)
  {
    uout << options.getArgv(i) << endl;;
    cGenomeDiff gd2(options.getArgv(i));
    gd1.merge(gd2, options.count("unique"), options.count("id"), options.count("verbose"));
  }
  
  uout("Writing output GD file", options["output"]);

  gd1.write(options["output"]);
  
  return 0;
}

int do_weights(int argc, char* argv[])
{
  AnyOption options("WEIGHTS -o <output.gd> <input1.gd input2.gd ...>");
  options("output,o",    "output GD file");
  options("verbose,v",   "verbose mode", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);

  options.addUsage("");
  options.addUsage("\tCreates a GD file of mutations and associated evidence that exist in each input GD file.");
  options.addUsage("Duplicate mutations are merged into a single mutation with a 'weight' field added to the");
  options.addUsage("entry. The 'weight' field is the inverse of the frequency of that mutation occuring accross");
  options.addUsage("all input GD files. Unique mutations will therefor have a 'weight' of 1");

  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }

  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("No additional input provided.");
    options.printUsage();
    return -1;
  }


  const bool verbose = options.count("verbose");

  UserOutput uout("WEIGHTS");

  uout("Readings input GD files"); 
  uout << options.getArgv(0) << endl;
  cGenomeDiff gd1(options.getArgv(0));

  //Load all the GD files that were input.
  for(int32_t i = 1; i < options.getArgc(); i++)
  {
    uout << options.getArgv(i) << endl;
    cGenomeDiff gd2(options.getArgv(i));
    gd1.fast_merge(gd2);
  }


  uout("Calculating weights");
  //Store entries as keys and pointers to matching entries.
  diff_entry_list_t muts = gd1.mutation_list();
  map<cDiffEntry, vector<diff_entry_ptr_t> > weights_table;

  for (diff_entry_list_t::iterator it = muts.begin(); 
       it != muts.end(); ++it) {
    cDiffEntry mut = **it;
    weights_table[mut].push_back(*it);
  }

  //Given the number of pointers, determine frequency and add 'weight' field to entries.
  for (map<cDiffEntry, vector<diff_entry_ptr_t> >::iterator it = weights_table.begin();
       it != weights_table.end(); ++it) {
    float weight = 1.0f / static_cast<float>(it->second.size());
    uout << weight << '\t' << it->first.to_spec() << endl;
    
    for (uint32_t i  = 0; i < it->second.size(); ++i) {
      (*it->second[i])["weight"] = to_string(weight);
    }

    if (verbose) {
      cout << "\tEntry: " << it->first << endl;
      cout << "\tMatches with:" << endl;
      for (uint32_t i  = 0; i < it->second.size(); ++i) {
        cout << "\t\t[entry]: " << (*it->second[i]) << endl;
      }
      cout << endl;
      cout << endl;
    }

  }
  uout("Assigning unique IDs");
  gd1.assign_unique_ids();
  
  uout("Writing output GD file", options["output"]);
  gd1.write(options["output"]);

  return 0;
}


int do_compare(int argc, char *argv[])
{
  AnyOption options("COMPARE -o <output.gd> <control.gd> <test.gd>");
  options("output,o",  "output GD file");
  options("verbose,v", "verbose mode", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);

  options.addUsage("");
  options.addUsage("   Given a control input GD file with valid mutations, compare each additional");
  options.addUsage("test GD file's mutations and determine if the mutations are true-postivie,");
  options.addUsage("false-negative or false-positive, where:");
  options.addUsage("");
  options.addUsage("       TP, true-positive  : mutation exists in both control and test GD files.");
  options.addUsage("       FN, false-negative : mutation exists in the control GD file but not in the test GD files.");
  options.addUsage("       FP, false-positive : mutation exists in one of the test GD files but not in the control GD file.");
  options.addUsage("");
  options.addUsage("The control and test GD files are merged into the given output GD file, for each");
  options.addUsage("mutation a 'compare' field will be added with a TP, FN, FP value. ");

  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }

  if (options.getArgc() < 2) {
    options.addUsage("");
    options.addUsage("Not enough input file provided.");
    options.printUsage();
    return -1;
  }

  UserOutput uout("COMPARE");

  uout("Reading input GD files");
  uout << "Control: " << options.getArgv(0) << endl;
  cGenomeDiff gd1(options.getArgv(0));
  uout << "Test:    " << options.getArgv(1) << endl;
  cGenomeDiff gd2(options.getArgv(1));


  uout("Comparing control vs test GD file");
  gd1.compare(gd2, options.count("verbose"));

  uout("Assigning unique IDs");
  gd1.assign_unique_ids();

  uout("Writing output GD file", options["output"]);
  gd1.write(options["output"]);

  return 0;
}

int do_convert_gvf( int argc, char* argv[]){
  AnyOption options("GD2GVF --input <input.gd> --output <output.gvf>"); 

  options
    ("help,h", "produce this help message", TAKES_NO_ARGUMENT)
    ("input,i","gd file to convert") 
    ("output,o","name of output file")
    ("snv-only", "only output SNV entries", TAKES_NO_ARGUMENT)
    ;
  options.processCommandArgs( argc,argv);

  options.addUsage("");
  options.addUsage("Creates a Genome Variant Format(GVF) file of mutations present in a input GD file.");
  
  if( !options.count("input") || !options.count("output") ){
      options.printUsage(); return -1;
  }
  
  try{
      GDtoGVF( options["input"], options["output"], options.count("snv-only") );
  } 
  catch(...){ 
      return -1; // failed 
  }
  
  return 0;
}

int do_convert_gd( int argc, char* argv[])
{
  AnyOption options("VCF2GD --input <intput.vcf> --output <output.gd>");
  options("input,i","gd file to convert");
  options("output,o","name of output file");
  options.processCommandArgs( argc,argv);

  options.addUsage("");
  options.addUsage("Creates a GD file of mutations present in a input Variant Call Format(VCF) file.");
  
  if(!options.count("input") && !options.count("output")){
    options.printUsage();
    return -1;
  }
  if(!options.count("input")) {
    options.printUsage();
    return -1;
  }
  
  cGenomeDiff gd = cGenomeDiff::from_vcf(options["input"]);
  const string &file_name = options.count("output") ?
        options["output"] :
        cString(options["input"]).remove_ending("vcf") + "gd";

  gd.write(file_name);

  return 0;
}

int do_not_evidence(int argc, char *argv[])
{
  AnyOption options("NOT_EVIDENCE -g <genomediff.gd> -o <output.gd>");
  options("genomediff,g","input GD files");
  options("output,o","output GD file");
  options("id,i","Reorder IDs (Flag)", TAKES_NO_ARGUMENT);
  options("verbose,v","Verbose Mode (Flag)", TAKES_NO_ARGUMENT);
  options.processCommandArgs(argc, argv);
  
  options.addUsage("");
  options.addUsage("Takes a GenomeDiff file and removes all of the entries that");
  options.addUsage("are NOT used as evidence by a mutation.  Outputs to a new");
  options.addUsage("GenomeDiff file if specified.  If no output is specified,");
  options.addUsage("verbose will still inform what evidence isn't being used.");
  
  if(argc == 1)  {
    options.printUsage();
    return -1;  }
  
  if (!options.count("genomediff")) {
    options.addUsage("");
    options.addUsage("You must supply the --genomediff option for input.");
    options.printUsage();
    return -1;
  }
  
  cGenomeDiff gd1(options["genomediff"]);
  
  gd1.filter_not_used_as_evidence(options.count("verbose"));
  
  if(options.count("output"))
  {
    if(options.count("id"))
    {
      cGenomeDiff gd2;
      gd2.merge(gd1, true, true);
      gd2.write(options["output"]);
    }
    else  {
      gd1.write(options["output"]);  }
  }
  
  return 0;
}

int do_annotate(int argc, char* argv[])
{
  AnyOption options("ANNOTATE [-o annotated.* --html] -r reference.gbk input.1.gd [input.2.gd ... ]");
  
  options
  ("help,h", "produce advanced help message", TAKES_NO_ARGUMENT)
  ("output,o", "path to output file with added mutation data. (DEFAULT: annotated.gd OR annotated.html")
  ("reference,r", "reference sequence in GenBank flatfile format (REQUIRED)")
  ("ignore-pseudogenes", "treats pseudogenes as normal genes for calling AA changes", TAKES_NO_ARGUMENT)
  ("html", "generate HTML output instead of GenomeDiff (REQUIRED)", TAKES_NO_ARGUMENT)
  ;
  options.addUsage("");
  options.addUsage("If multiple GenomeDiff input files are provided, then they are");
  options.addUsage("merged and the frequencies from each file shown for each mutation.");

  options.processCommandArgs(argc, argv);
  
  UserOutput uout("ANNOTATE");
  
  bool html_output_mode = options.count("html");
  string output_file_name;
  if (options.count("output")) 
    output_file_name = options["output"];
  else
    output_file_name = (html_output_mode) ? "annotated.html" : "annotated.gd";
  
  vector<string> gd_path_names;
  for (int32_t i = 0; i < options.getArgc(); ++i) {
    gd_path_names.push_back(options.getArgv(i));
  }
  
  if ( (gd_path_names.size() == 0) 
    || !options.count("reference")){
    options.printUsage();
    return -1;
  }
  
  // more than one file was provided as input
  bool compare_mode = (gd_path_names.size() > 1);
  
  vector<string> gd_base_names;
  for (uint32_t i = 0; i < gd_path_names.size(); i++){    
    cString s(gd_path_names[i]);
    s = s.get_base_name_no_extension();
    gd_base_names.push_back(s);
  }
  
  // First use merge to produce a file with a line for each mutation
  cGenomeDiff gd;
  vector<diff_entry_list_t> mut_lists;
  vector<cGenomeDiff> gd_list;

  for (uint32_t i = 0; i < gd_path_names.size(); i++){
    uout("Reading input GD file",gd_path_names[i]);
    cGenomeDiff single_gd(gd_path_names[i]);
    gd_list.push_back(single_gd);
    mut_lists.push_back(single_gd.mutation_list());
    gd.merge(single_gd);
  }
  gd.sort();
  
  // Then add frequency columns for all genome diffs
  if (compare_mode) {
    
    diff_entry_list_t de_list = gd.mutation_list();
    bool found = false;
    
    for (diff_entry_list_t::iterator it = de_list.begin(); it != de_list.end(); it++) { 

      diff_entry_ptr_t& this_mut = *it;
    
      // for each genome diff compared
      for (uint32_t i=0; i<mut_lists.size(); i++) { 
		
        string freq_key = "frequency_" + gd_base_names[i];
        (*this_mut)[freq_key] = "0";

        diff_entry_list_t& mut_list = mut_lists[i];
        if (mut_list.size() == 0) 
          continue; 
        
        bool found = false;
        
        // for top mutation in this genomedff (they are sorted by position)
        diff_entry_ptr_t check_mut;
        check_mut = mut_list.front();        
        
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
  

  vector<string> reference_file_names = from_string<vector<string> >(options["reference"]);
  uout("Reading input reference sequence files") << reference_file_names << endl;
  cReferenceSequences ref_seq_info;
  ref_seq_info.LoadFiles(reference_file_names);
    
  uout("Annotating mutations");
  ref_seq_info.annotate_mutations(gd, true, options.count("ignore-pseudogenes"));
    
  if (html_output_mode) {
    
    uout("Writing output HTML file", output_file_name);
    
    Settings settings;
    // No evidence needs to be transferred to options and initialized correctly within breseq
    settings.no_evidence = true;
    
    MutationTableOptions mt_options;
    if (compare_mode)
      mt_options.repeat_header = true;
    mt_options.one_ref_seq = ref_seq_info.size() == 1;
    mt_options.gd_name_list_ref = gd_base_names;
    mt_options.repeat_header = 10;
    
    html_compare(settings, output_file_name, "Mutation Comparison", gd, mt_options);
        
  } else {
    uout("Writing output HTML file", options["output"]);
    gd.write(output_file_name);
  }
  
  return 0;
}

int do_count(int argc, char* argv[])
{
  AnyOption options("COUNT [-o annotated.*] -r reference.gbk input.1.gd [input.2.gd ... ]");
  
  options
  ("help,h", "produce advanced help message", TAKES_NO_ARGUMENT)
  ("output,o", "path to output file with added mutation data.", "count.csv")
  ("reference,r", "reference sequence in GenBank flatfile format (REQUIRED)")
  ("ignore-pseudogenes", "treats pseudogenes as normal genes for calling AA changes", TAKES_NO_ARGUMENT)
  ;
  options.addUsage("");
  options.addUsage("Counts the numbers of mutations and other statistics for each input GenomeDiff file.");
  
  options.processCommandArgs(argc, argv);
  
  UserOutput uout("COUNT");
  
  string output_file_name = options["output"];

  vector<string> gd_path_names;
  for (int32_t i = 0; i < options.getArgc(); ++i) {
    gd_path_names.push_back(options.getArgv(i));
  }
  
  // Perl that needs to be ported.
  /*
   my $base_substitution_file;
   my @reference_genbank_file_names;
   GetOptions(
   'help|?' => \$help, 'man' => \$man,
   ## Options for input and output files
   'output|o=s' => \$output,	
   'reference-sequence|r=s' => \@reference_genbank_file_names,
   'base-substitution-file|s=s' => \$base_substitution_file,
   ) or pod2usage(2);
   
   pod2usage(1) if $help;
   pod2usage(-exitstatus => 0, -verbose => 2) if $man;
   pod2usage(1) if (scalar @reference_genbank_file_names == 0);
   pod2usage(1) if (scalar @ARGV == 0);
   
   my @gd_file_name_list = @ARGV;	
   
   ## load information about reference sequences from GenBank files
   my $ref_seq_info = load_ref_seq_info(\@reference_genbank_file_names);
   my $one_ref_seq = scalar keys %{$ref_seq_info->{gene_lists}} == 1;
   
   my $total_bp = 0;
   
   foreach my $seq_id (keys %{$ref_seq_info->{ref_strings}})
   {
   $total_bp += length $ref_seq_info->{ref_strings}->{$seq_id};
   }
   
   ## how many repeat columns
   my $mob_name_hash;
   my $con_name_hash;
   foreach my $gd_file_name (@gd_file_name_list)
   {
   print "$gd_file_name\n";
   
   my $gd_name = $gd_file_name;
   $gd_name =~ s/^.+\///;		
   $gd_name =~ s/\.[^.]+$//;		
   
   my $gd = GenomeDiff->new({in=>$gd_file_name});
   foreach my $mut ($gd->list('MOB'))
   {
   $mob_name_hash->{$mut->{repeat_name}} = 1;
   }
   foreach my $mut ($gd->list('CON'))
   {
   $con_name_hash->{$mut->{mediated}} = 1 if ($mut->{mediated});
   }
   foreach my $mut ($gd->list('DEL'))
   {
   $mob_name_hash->{$mut->{mediated}} = 1 if ($mut->{mediated});
   }
   
   }
   my @mod_name_list = sort keys %$mob_name_hash;
   my @con_name_list = sort keys %$con_name_hash;
   
   
   my $main_seq_id = $ref_seq_info->{seq_ids}->[0];
   my $main_seq_length = length $ref_seq_info->{ref_strings}->{$main_seq_id};
   
   
   my $bsf;
   my $bs_totals;
   if ($base_substitution_file)
   {
   $bsf = GenomeDiff::BaseSubstitutionFile->read(input_file => $base_substitution_file, nt_sequence => $ref_seq_info->{ref_strings}->{$main_seq_id});
   $bs_totals = $bsf->totals;
   print Dumper($bs_totals);
   }
   
   ##not by base change
   my @snp_types = ( 'nonsynonymous', 'synonymous', 'noncoding', 'pseudogene', 'intergenic');
   
   open OUT, ">$output" or die "Count not open file: $output";
   
   my @column_headers = (
   'strain', 
   'total', 
   @snp_types, 
   'small_indel',
   'large_del',
   'deleted_bp',
   'inserted_bp', 
   'repeat_inserted_bp',
   'amplification',
   'mobile_element',
   @mod_name_list,
   'gene_conversion',
   @con_name_list,
   'called_bp', 
   'total_bp',
   );
   
   if ($base_substitution_file) {
   
   foreach my $snp_type (@GenomeDiff::BaseSubstitutionFile::bsf_snp_types) {
   foreach my $bp_change (@GenomeDiff::BaseSubstitutionFile::bp_change_label_list, 'TOTAL') {
   push @column_headers, "POSSIBLE.$snp_type\.$bp_change";
   }
   }
   foreach my $snp_type (@GenomeDiff::BaseSubstitutionFile::bsf_snp_types) {
   foreach my $bp_change (@GenomeDiff::BaseSubstitutionFile::bp_change_label_list, 'TOTAL') {
   push @column_headers, "OBSERVED.$snp_type\.$bp_change";
   }
   }
   }
   
   print OUT join ("\t", @column_headers) . "\n";
   
   
   ## count SNPs and examples of mobile element insertions
   
   my @gd_name_list;
   my @gd_list; 
   my @mut_lists; #altered during processing
   foreach my $gd_file_name (@gd_file_name_list)
   {
   print "$gd_file_name\n";
   
   my $gd_name = $gd_file_name;
   $gd_name =~ s/^.+\///;		
   $gd_name =~ s/\.[^.]+$//;		
   
   my $gd = GenomeDiff->new({in=>$gd_file_name});
   GenomeDiff::ReferenceSequence::annotate_mutations($ref_seq_info, $gd, 1);
   
   #deep copy sums
   my $this_bs_totals;
   if ($base_substitution_file) {
   $this_bs_totals = dclone($bs_totals);
   }
   
   my @mut_list = $gd->mutation_list();
   @mut_list = grep {!$_->{deleted}} @mut_list;
   
   my $count;
   
   
   foreach my $st (@snp_types)
   {
   $count->{type}->{$st} = 0;
   }
   foreach my $mob_name (@mod_name_list)
   {
   $count->{mob}->{$mob_name} = 0;
   }
   
   my $total_deleted = 0;
   my $total_inserted = 0;
   my $total_repeat_inserted = 0;
   
   my $this_bs_counts = [];
   foreach my $mut (@mut_list)
   {
   if ($mut->{type} eq 'SNP')
   {				
   my $base_change = $mut->{_ref_seq} . "." . $mut->{new_seq};
   my $bp_change = $GenomeDiff::BaseSubstitutionFile::base_to_bp_change->{$base_change};				
   $count->{type}->{$mut->{snp_type}}++;				
   
   ## SCF snp count	
   if ($base_substitution_file) {
   $this_bs_counts = $bsf->add_bp_change_to_totals($this_bs_counts, $mut->{position}, $bp_change);					
   }
   }
   
   if ($mut->{type} eq 'DEL')
   {
   $total_deleted += $mut->{size};
   $count->{mob}->{$mut->{mediated}}++ if ($mut->{mediated});
   
   if ($mut->{size} > 20) 
   {
   $count->{large_del}++;
   }
   else 
   {
   $count->{small_indel}++;
   }
   }
   
   if ($mut->{type} eq 'INS')
   {
   $total_inserted += $mut->{size};
   $count->{mob}->{$mut->{mediated}}++ if ($mut->{mediated});
   
   if ($mut->{size} <= 20) 
   {
   $count->{small_indel}++;
   }
   }
   
   if ($mut->{type} eq 'CON')
   {
   #need calculation of size change here
   $count->{con}->{$mut->{mediated}}++ if ($mut->{mediated});
   $count->{gene_conversion}++;
   }
   
   if ($mut->{type} eq 'MOB')
   {
   #this returns shortest example				
   my $repeat_seq = GenomeDiff::ReferenceSequence::repeat_example($ref_seq_info, $mut->{repeat_name}, 1);
   my $this_length = length($repeat_seq);
   $total_inserted += $this_length;
   $total_repeat_inserted += $this_length;
   print "Repeat $mut->{repeat_name} $this_length bp\n";
   
   $count->{mob}->{$mut->{repeat_name}}++;
   $count->{mobile_elements}++;
   }			
   
   if ($mut->{type} eq 'AMP')
   {
   $total_inserted += $mut->{size};
   if ($mut->{size} > 20)
   {
   $count->{amp}++;
   }
   else
   {
   $count->{small_indel}++;
   }
   }
   }
   
   ##statistics for UN
   my $un_bp = 0; 
   foreach my $un ($gd->list('UN'))
   {
   $un_bp += $un->{end} - $un->{start} + 1;
   if ($base_substitution_file)
   {
   my $ref_string = $ref_seq_info->{ref_strings}->{$main_seq_id};
   
   for (my $p = $un->{start}; $p <= $un->{end}; $p++)
   {
   ### subroutine to subtract certain position info
   $bsf->subtract_position_1_from_totals($this_bs_totals, $p);					
   }
   }
   }
   
   print Dumper($bs_totals);
   
   my $called_bp = $total_bp - $un_bp;
   
   my @this_columns = (
   $gd_name, 
   scalar(@mut_list),  
   map({$count->{type}->{$_} } @snp_types),
   defined $count->{small_indel} ? $count->{small_indel} : "0",
   defined $count->{large_del} ? $count->{large_del} : "0",
   $total_deleted, 
   $total_inserted, 
   $total_repeat_inserted,
   defined $count->{amp} ? $count->{amp} : "0",
   defined $count->{mobile_elements} ? $count->{mobile_elements} : "0",
   map({$count->{mob}->{$_}} @mod_name_list), 
   defined $count->{gene_conversion} ? $count->{gene_conversion} : "0",
   map({$count->{con}->{$_}} @con_name_list), 
   $called_bp, 
   $total_bp
   );
   
   
   if ($base_substitution_file) {			
   for (my $i = 0; $i < scalar @$this_bs_totals; $i++) {
   foreach my $bp_change (@GenomeDiff::BaseSubstitutionFile::bp_change_label_list, 'TOTAL') {
   push @this_columns, (defined $this_bs_totals->[$i]->{$bp_change}) ? $this_bs_totals->[$i]->{$bp_change} : '0';
   }
   }
   
   
   for (my $i = 0; $i < scalar @$this_bs_counts; $i++) {
   foreach my $bp_change (@GenomeDiff::BaseSubstitutionFile::bp_change_label_list, 'TOTAL') {
   push @this_columns, (defined $this_bs_counts->[$i]->{$bp_change}) ? $this_bs_counts->[$i]->{$bp_change} : '0';
   }
   }
   }
   
   print OUT join("\t", @this_columns) . "\n";
   #print Dumper ($count);
   }	
   print "Done\n";
   close OUT;
   */
  
  return 0;
}

int do_normalize_gd(int argc, char* argv[])
{
  AnyOption options("NORMALIZE -o <output.gd> -r <reference> <input1.gd input2.gd input3.gd ...>");
  options
  ("output,o"     , "output GD file.")
  ("reference,r"  , "input reference file.")
  ("verbose,v"    , "Verbose Mode (Flag)", TAKES_NO_ARGUMENT);

  options.processCommandArgs(argc, argv);
  options.addUsage("");
  options.addUsage("Creates a GD file of mutations that have been normalized to the input reference files.");

  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("No output provided.");
    options.printUsage();
    return -1;
  }

  if (!options.count("reference")) {
    options.addUsage("");
    options.addUsage("No reference provided.");
    options.printUsage();
    return -1;
  }

  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("No additional input provided.");
    options.printUsage();
    return -1;
  }

  if (!options.count("output")) {
    options.addUsage("");
    options.addUsage("You must supply the --output option for output.");
    options.printUsage();
    return -1;
  }

  bool verbose = options.count("verbose");

  UserOutput uout("NORMALIZE");

  vector<string> rfns = from_string<vector<string> >(options["reference"]);
  uout("Reading in reference sequence files") << rfns << endl;
  cReferenceSequences rs;
  rs.LoadFiles(rfns);

  uout("Merging input GD files") <<  options.getArgv(0) << endl;
  cGenomeDiff gd(options.getArgv(0));

  for (int32_t i = 1; i < options.getArgc(); ++i) {
    uout << options.getArgv(i) << endl;
    cGenomeDiff temp_gd(options.getArgv(i));
    gd.merge(temp_gd, false, false, options.count("verbose"));
  }

  uout("Normalizing mutations");
  gd.normalize_to_sequence(rs);
  diff_entry_list_t muts = gd.mutation_list();

  for (diff_entry_list_t::iterator it = muts.begin();
       it != muts.end(); it++) {
    diff_entry_ptr_t mut = *it;
    if (mut->entry_exists("norm") && (*mut)["norm"] == "is_not_valid"){
      uout << "INVALID_MUTATION\t" << **it << endl; 
      (*mut)["comment_out"] = "True";
    } 
  }

  uout("Writing output GD file", options["output"]);
  gd.write(options["output"]);

  return 0;
}

int do_filter_gd(int argc, char* argv[]) {
  AnyOption options("FILTER -i <input.gd> -o <output.gd> -m <mutation type> <filters>");
  options("input,i", "Genome diff file to be filtered.");
  options("output,o", "Filter genome diff file.");
  options("mut_type,m", "Only consider this mutation type for filtering.");
  options.processCommandArgs(argc, argv);

  options.addUsage("");
  options.addUsage("Creates a GD file of mutations that evaluate as false to the input filtering expressions.");

  if (!options.count("input")) {
    options.addUsage("");
    options.addUsage("You must supply an input genome diff file to filter with option -g");
    options.printUsage();
    return -1;
  }

  if (!options.getArgc()) {
    options.addUsage("");
    options.addUsage("You must supply filter arguments.");
    options.printUsage();
    return -1;
  }

  // Use original file as output if argument is not passed.
  if (!options.count("output")) {
    options["output"] = options["input"];
  }

  UserOutput uout("FILTER");

  list<string> filters;
  for(int32_t i = 0; i < options.getArgc(); ++i) {
    filters.push_back(options.getArgv(i));
  }
  assert(filters.size());

  cGenomeDiff gd(options["input"]);

  diff_entry_list_t muts;
  if (!options.count("mut_type")) {
    muts = gd.mutation_list();
  }
  else {
    vector<string> mut_strs= from_string<vector<string> >(options["mut_type"]);
    assert (mut_strs.size());
    vector<gd_entry_type> mut_types;
    for (size_t i = 0; i < mut_strs.size(); ++i) {
      for (size_t j = 0; j < gd_entry_type_lookup_table.size(); ++j) {
        if (gd_entry_type_lookup_table[j] == mut_strs[i]) {
          mut_types.push_back((gd_entry_type)j);
          break;
        }
      }
    }
    muts = gd.list(mut_types);
  }
  if (!muts.size()) {
    printf("No mutations found.\n");
    return 0;
  }

  cGenomeDiff output_gd;
  const vector<string> evals = make_vector<string>("==")("!=")("<=")(">=")("<")(">");
  for (diff_entry_list_t::iterator it = muts.begin(); it != muts.end(); ++it) {
    cDiffEntry mut = **it;

    vector<string> reasons;
    for (list<string>:: const_iterator jt = filters.begin(); jt != filters.end(); ++jt) {
      bool is_filtered = false;
      // Parse filter string.
      string filter = *jt;
      for (size_t i = 0; i < filter.size(); ++i) {
        if (filter[i] == ' ') {
          filter.erase(i,1);
        }
      }
      string key   = "";
      size_t eval  = string::npos;
      string value = "";
      for (size_t i = 0; i < evals.size(); ++i) {
        if (filter.find(evals[i]) != string::npos) {
          size_t pos = filter.find(evals[i]);
          eval = i;
          string segment = filter;
          segment.erase(pos, evals[i].size());
          key = segment.substr(0, pos);
          value = segment.substr(pos);
          break;
        }
      }
      assert(eval != string::npos);
      assert(key.size());
      assert(value.size());

      if (mut.count(key)) {
        if (value.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") == string::npos) {
          switch(eval)
          {
            case 0: if (from_string<float>(mut[key]) == from_string<float>(value)) is_filtered = true; break;
            case 1: if (from_string<float>(mut[key]) != from_string<float>(value)) is_filtered = true; break;
            case 2: if (from_string<float>(mut[key]) <= from_string<float>(value)) is_filtered = true; break;
            case 3: if (from_string<float>(mut[key]) >= from_string<float>(value)) is_filtered = true; break;
            case 4: if (from_string<float>(mut[key]) <  from_string<float>(value)) is_filtered = true; break;
            case 5: if (from_string<float>(mut[key]) >  from_string<float>(value)) is_filtered = true; break;
          };

          if (is_filtered) {
            reasons.resize(reasons.size() + 1);
            sprintf(reasons.back(), "%s %s %s", key.c_str(), evals[eval].c_str(), value.c_str());
          }
        }
      }
    }
    if (reasons.size()) {
      printf("Filtered[%s]: %s\n", join(reasons, ", ").c_str(), mut.to_string().c_str());
      mut["filtered"] = join(reasons, ", ");
      mut["comment_out"] = "true";
    }
    output_gd.add(mut);
  }
  uout("Writing output GD file", options["output"]);
  output_gd.write(options["output"]);
  return 0;
}

int main(int argc, char* argv[]) {
	//Extract the sub-command argument.
	string command;
	char* argv_new[argc];
	int argc_new = argc - 1;

  //Print out our generic header.
  Settings::command_line_run_header();
                                      
	if (argc > 1) {

		command = argv[1];
		argv_new[0] = argv[0];
		for (int32_t i = 1; i < argc; i++)
			argv_new[i] = argv[i + 1];

	} else {
    gdtools_usage();
    return -1; 
	}

	//Pass the command to the proper handler.
	command = to_upper(command);

  // Genome Diff Commands:
  if (command == "APPLY") {
    return do_mutate(argc_new, argv_new);    
  } else if (command == "COMPARE") {
    return do_compare(argc_new, argv_new);
  } else if (command == "NOT-EVIDENCE") {        //TODO merge with FILTER
    return do_not_evidence(argc_new, argv_new);
  } else if (command == "ANNOTATE") {
    return do_annotate(argc_new, argv_new);
  } else if (command == "COUNT") {
    return do_count(argc_new, argv_new);
  } else if (command == "NORMALIZE") {
    return do_normalize_gd(argc_new, argv_new);
  } else if (command == "FILTER") {
    return do_filter_gd(argc_new, argv_new);

  } else if (command == "INTERSECT") {
    return do_intersection(argc_new, argv_new);
  } else if (command == "UNION") {
    return do_union(argc_new, argv_new);
  } else if (command == "SUBTRACT") {
    return do_subtract(argc_new, argv_new);    
  } else if (command == "MERGE") {
    return do_merge(argc_new, argv_new);    
  } else if (command == "WEIGHTS") {
    return do_weights(argc_new, argv_new);
  } else if (command == "GD2GVF") {             
    return do_convert_gvf(argc_new, argv_new);
  } else if (command == "VCF2GD") {             
    return do_convert_gd( argc_new, argv_new);
  }

  return 0;

}

