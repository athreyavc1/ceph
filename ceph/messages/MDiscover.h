#ifndef __MDISCOVER_H
#define __MDISCOVER_H

#include "include/Message.h"
#include "mds/CDir.h"
#include "include/filepath.h"

#include <vector>
#include <string>
using namespace std;


class MDiscover : public Message {
  int             asker;
  inodeno_t       base_ino;          // 0 -> none, want root
  bool            want_base_dir;
  
  dirpath         want;   // ... [/]need/this/stuff

 public:
  int get_asker() { return asker; }
  inodeno_t get_base_ino() { return base_ino; }
  filepath& get_want() { return want; }
  string&  get_dentry(int n) { return want[n]; }

  MDiscover() { }
  MDiscover(int asker, 
			inodeno_t base_ino,
			filepath& want,
			bool want_base_dir = true) :
	Message(MSG_MDS_DISCOVER) {
	this->asker = asker;
	this->base_ino = base_ino;
	this->want = want;
	this-.want_base_dir = want_base_dir;
  }
  virtual char *get_type_name() { return "Dis"; }

  virtual int decode_payload(crope r) {
	r.copy(0, sizeof(asker), (char*)&asker);
	basepath = r.c_str() + sizeof(asker);
	unsigned off = sizeof(asker) + basepath.length() + 1;
	
	want = new vector<string>;
	int num_want;
	r.copy(off, sizeof(int), (char*)&num_want);
	off += sizeof(int);
	for (int i=0; i<num_want; i++) {
	  string w = r.c_str() + off;
	  off += w.length() + 1;
	  want->push_back(w);
	}
	
	int ntrace;
	r.copy(off, sizeof(int), (char*)&ntrace);
	off += sizeof(int);
	for (int i=0; i<ntrace; i++) {
	  MDiscoverRec_t dr;
	  off += dr._unrope(r.substr(off, r.length()));
	  trace.push_back(dr);
	}
  }
  virtual crope get_payload() {
	crope r;
	r.append((char*)&asker, sizeof(asker));
	r.append(basepath.c_str());
	r.append((char)0);

	if (!want) want = new vector<string>;
	int num_want = want->size();
	r.append((char*)&num_want,sizeof(int));
	for (int i=0; i<num_want; i++) {
	  r.append((*want)[i].c_str());
	  r.append((char)0);
	}
	
	int ntrace = trace.size();
	r.append((char*)&ntrace, sizeof(int));
	for (int i=0; i<ntrace; i++)
	  r.append(trace[i]._rope());
	
	return r;
  }

  
  // ---

  void add_bit(CInode *in, int auth, int nonce) {
	MDiscoverRec_t bit;

	assert(nonce >= 0);

	bit.inode = in->inode;
	bit.cached_by = in->get_cached_by();
	bit.cached_by.insert( auth );  // obviously the authority has it too
	bit.dir_auth = in->dir_auth;
	bit.replica_nonce = nonce;

	// send sync/lock state
	bit.is_syncbyauth = in->is_syncbyme() || in->is_presync();
	bit.is_softasync = in->is_softasync();
	bit.is_lockbyauth = in->is_lockbyme() || in->is_prelock();

	if (in->is_dir() && in->dir) {
	  bit.dir_rep = in->dir->dir_rep;
	  bit.dir_rep_by = in->dir->dir_rep_by;
	}

	trace.push_back(bit);
  }

  string current_base() {
	string c = basepath;
	for (int i=0; i<trace.size(); i++) {
	  c += "/";
	  c += (*want)[i];
	}
	return c;
  }

  string current_need() {
	if (just_root())
	  return string("");  // just root

	string a = current_base();
	a += "/";
	a += next_dentry();
	return a;
  }

  string& next_dentry() {
	return (*want)[trace.size()];
  }

  bool just_root() {
	if (want == NULL ||
		want->size() == 0) return true;
	return false;
  }
  
  bool done() {
	// just root?
	if (just_root() &&
		trace.size() > 0) return true;

	// normal
	if (trace.size() == want->size()) return true;
	return false;
  }
};

#endif
