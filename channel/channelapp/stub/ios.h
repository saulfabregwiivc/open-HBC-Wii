
struct ioctlv {
	void *data;
	u32 len;
};

int ios_open(const char *filename, u32 mode);
int ios_close(int fd);
int ios_ioctl(int fd, u32 n, void *in, u32 in_size, void *out, u32 out_size);
int ios_ioctlv(int fd, u32 n, u32 in_count, u32 out_count, struct ioctlv *vec);
int ios_ioctlvreboot(int fd, u32 n, u32 in_count, u32 out_count, struct ioctlv *vec);
void reset_ios(void);
