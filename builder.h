#ifndef __BUILDER_H
#define __BUILDER_H

typedef struct InfrastructureSpec InfrastructureSpec;

InfrastructureSpec* new_infrastructurespec(void);
void delete_infrastructurespec(InfrastructureSpec* is);

void add_signal(InfrastructureSpec* is, const char* name, 
		int upIdx, double upLength,
		int downIdx, double downLength,
		int upDir);
  void add_detector(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength,
		  int upTvd, int downTvd);
  void add_sight(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength,
		  size_t sigref, double dist);
  void add_boundary(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength);
  void add_stop(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength);
  void add_tvd(InfrastructureSpec* is, const char* name);
  void add_switch(InfrastructureSpec* is, const char* name,
		  int upIdx1, double upLength1, 
		  int upIdx2, double upLength2, 
		  int downIdx1, double downLength1,
		  int downIdx2, double downLength2,
		  int state);

#endif
