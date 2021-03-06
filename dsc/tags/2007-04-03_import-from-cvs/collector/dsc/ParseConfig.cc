
#include <iostream>
#include <fstream>
#include <assert.h>

#include <Hapy/Parser.h>
#include <Hapy/Rules.h>
#include <Hapy/IoStream.h>

#include <errno.h>

extern "C" int add_local_address(const char *);
extern "C" int open_interface(const char *);
extern "C" int set_run_dir(const char *);
extern "C" int set_pid_file(const char *);
extern "C" int add_dataset(const char *name, const char *layer,
	const char *firstname, const char *firstindexer,
	const char *secondname, const char *secondindexer,
	const char *filtername, int min_count, int max_cells);
extern "C" int set_bpf_vlan_tag_byte_order(const char *);
extern "C" int set_bpf_program(const char *);
extern "C" int set_match_vlan(const char *);
extern "C" int add_qname_filter(const char *name, const char *re);

extern "C" void ParseConfig(const char *);

using namespace Hapy;

enum {
	ctBareToken = 1,
	ctQuotedToken,
	ctToken,
	ctDecimalNumber,
	ctComment,
	ctIPv4Address = 11,
	ctHostOrNet,
	ctInterface = 21,
	ctRunDir,
	ctPidFile,
	ctLocalAddr,
	ctPacketFilterProg,
	ctDataset,
	ctDatasetOpt,
	ctBVTBO,		// bpf_vlan_tag_byte_order
	ctMatchVlan,
	ctQnameFilter,
	ctConfig = 30,
	ctMax
} configToken;

Rule rBareToken;
Rule rQuotedToken;
Rule rToken;
Rule rDecimalNumber;
Rule rComment;

Rule rIPv4Address;
Rule rHostOrNet;

Rule rInterface("Interface", 0);
Rule rRunDir("RunDir", 0);
Rule rPidFile("PidFile", 0);
Rule rLocalAddr("LocalAddr", 0);
Rule rPacketFilterProg("PacketFilterProg", 0);
Rule rDatasetOpt("DatasetOpt", 0);
Rule rDataset("Dataset", 0);
Rule rBVTBO("BVTBO", 0);
Rule rMatchVlan("MatchVlan", 0);
Rule rQnameFilter("QnameFilter", 0);

Rule rConfig;

string
remove_quotes(const string &s) 
{
	string::size_type f = s.find_first_of('"');
	string::size_type l = s.find_last_of('"');
	return s.substr(f+1, l-f-1);
}

static int
getDatasetOptVal(const Pree tree, string name, int &val)
{
	for (unsigned int i=0; i<tree.count(); i++) {
		if (0 != tree[i][0].image().compare(name))
			continue;
		val = atoi(tree[i][2].image().c_str());
		return true;
	}
	return false;
}


/* interpret()
 *
 * I'm a recursive function.
 */
static int
interpret(const Pree &tree, int level)
{
	int x;
        if (tree.rid() == rInterface.id()) {
		assert(tree.count() > 1);
                if (open_interface(tree[1].image().c_str()) != 1) {
			cerr << "interpret() failure in interface" << endl;
			return 0;
		}
	} else
        if (tree.rid() == rRunDir.id()) {
		assert(tree.count() > 1);
                if (set_run_dir(remove_quotes(tree[1].image()).c_str()) != 1) {
			cerr << "interpret() failure in run_dir" << endl;
			return 0;
		}
	} else
        if (tree.rid() == rPidFile.id()) {
		assert(tree.count() > 1);
                if (set_pid_file(remove_quotes(tree[1].image()).c_str()) != 1) {
			cerr << "interpret() failure in pid_file" << endl;
			return 0;
		}
	} else
        if (tree.rid() == rLocalAddr.id()) {
		assert(tree.count() > 1);
		if (add_local_address(tree[1].image().c_str()) != 1) {
			cerr << "interpret() failure in local_addr" << endl;
			return 0;
		}
        } else
	if (tree.rid() == rDataset.id()) {
		int min_count = 0;
		int max_cells = 0;
		assert(tree.count() > 10);
		for (unsigned int i=10; i<tree.count(); i++) {
			getDatasetOptVal(tree[i], "min-count", min_count);
			getDatasetOptVal(tree[i], "max-cells", max_cells);
		}
		x = add_dataset(tree[1].image().c_str(),	// name
			tree[2].image().c_str(),		// layer
			tree[3].image().c_str(),		// 1st dim name
			tree[5].image().c_str(),		// 1st dim indexer
			tree[6].image().c_str(),		// 2nd dim name
			tree[8].image().c_str(),		// 2nd dim indexer
			tree[9].image().c_str(),		// filter name
			min_count,				// min cell count to report
			max_cells);				// max 2nd dim cells to print
		if (x != 1) {
			cerr << "interpret() failure in dataset" << endl;
			return 0;
		}
        } else
	if (tree.rid() == rBVTBO.id()) {
		assert(tree.count() > 1);
		if (set_bpf_vlan_tag_byte_order(tree[1].image().c_str()) != 1) {
			cerr << "interpret() failure in bpf_vlan_tag_byte_order" << endl;
			return 0;
		}
	} else
	if (tree.rid() == rMatchVlan.id()) {
		for(unsigned int i = 0; i<tree[1].count(); i++) {
			if (set_match_vlan(tree[1][i].image().c_str()) != 1) {
				cerr << "interpret() failure in match_vlan" << endl;
				return 0;
			}
		}
	} else
	if (tree.rid() == rPacketFilterProg.id()) {
		assert(tree.count() > 1);
		if (set_bpf_program(remove_quotes(tree[1].image()).c_str()) != 1) {
			cerr << "interpret() failure in bpf_filter_prog" << endl;
			return 0;
		}
	} else
	if (tree.rid() == rQnameFilter.id()) {
		assert(tree.count() > 2);
		if (add_qname_filter(tree[1].image().c_str(), tree[2].image().c_str()) != 1) {
			cerr << "interpret() failure in qname_filter" << endl;
			return 0;
		}
	} else
        {
                for (unsigned int i = 0; i < tree.count(); i++) {
                        if (interpret(tree[i], level + 1) != 1) {
			cerr << "interpret() failure in recurse" << endl;
				return 0;
			}
                }
        }
	return 1;
}

void
ParseConfig(const char *fn)
{
	// primitive token level
	rBareToken = +(anychar_r - space_r - "\"" - ";");
	rQuotedToken = quoted_r(anychar_r);
	rToken = rBareToken | rQuotedToken;
	rDecimalNumber = +digit_r;
	rComment = quoted_r(anychar_r, '#', eol_r);

	// fancy token level
	rIPv4Address = rDecimalNumber >> "." >>
		rDecimalNumber >> "." >>
		rDecimalNumber >> "." >>
		rDecimalNumber;
	rHostOrNet = string_r("host") | string_r("net");


	// rule/line level
	rInterface = "interface" >>rBareToken >>";" ;
	rRunDir = "run_dir" >>rQuotedToken >>";" ;
	rPidFile = "pid_file" >>rQuotedToken >>";" ;
	rLocalAddr = "local_address" >>rIPv4Address >>";" ;
	rPacketFilterProg = "bpf_program" >>rQuotedToken >>";" ;
	rDatasetOpt = rBareToken >> "=" >> rDecimalNumber;
	rDataset = "dataset" >>rBareToken >>rBareToken
		>>rBareToken >>":" >>rBareToken
		>>rBareToken >>":" >>rBareToken
		>>rBareToken
		>>*rDatasetOpt >>";" ;
	rBVTBO = "bpf_vlan_tag_byte_order" >>rHostOrNet >>";" ;
	rMatchVlan = "match_vlan" >> +rDecimalNumber >>";" ;
	rQnameFilter = "qname_filter" >>rBareToken >>rBareToken >>";" ;

	// the whole config
	rConfig = *(
		rInterface |
		rRunDir |
		rPidFile |
		rLocalAddr |
		rPacketFilterProg |
		rDataset |
		rBVTBO |
		rMatchVlan |
		rQnameFilter
	) >> end_r;
	rConfig.Debug(false);

	// trimming - do not allow whitespace INSIDE these objects
	rConfig.trim(*(space_r | rComment));
	rQuotedToken.verbatim(true);
	rBareToken.verbatim(true);
	rDecimalNumber.verbatim(true);
	rIPv4Address.verbatim(true);

	// parse tree shaping
	rQuotedToken.leaf(true);
	rBareToken.leaf(true);
	rDecimalNumber.leaf(true);
	rIPv4Address.leaf(true);
	rHostOrNet.leaf(true);

	// commit points
        rInterface.committed(true);
        rRunDir.committed(true);
        rLocalAddr.committed(true);
        rPacketFilterProg.committed(true);
        rDataset.committed(true);
	rBVTBO.committed(true);
	rMatchVlan.committed(true);

	std::string config;
	char c;
	std::ifstream in(fn);
	if (!in) {
		cerr << fn << ": " << strerror(errno) << endl;
		exit(1);
	}
	while (in.get(c))
		config += c;
	in.close();
	if (config.empty()) {
		cerr << "Did not find configuration in " << fn << endl;
		exit(1);
	}

	Parser parser;
	parser.grammar(rConfig);
	if (!parser.parse(config)) {
		parser.result().pree.print(cerr << "complete failure" << endl, "");
		string::size_type p = parser.result().maxPos;
		cerr << "parse failure after " << p
			<< " bytes, near "
		       << (p >= config.size() ?
			       string(" the end of input") : config.substr(p).substr(0, 160))
		       << endl;
		exit(1);
	}
	if (interpret(parser.result().pree, 0) != 1) {
		cerr << "Failed to correctly INTERPRET parsed config file" << endl;
		abort();
	}
}
