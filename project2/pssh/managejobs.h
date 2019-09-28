typedef enum {
	STOPPED,
	TERM,
	BG,
	FG,
} JobStatus;

typedef struct {
	char* name;
	pid_t* pids;
	unsigned int npids;
	unsigned int pids_left;
	pid_t pgid;
	JobStatus status;
} Job;