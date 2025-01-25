#include <bfd.h>
#include <elf.h>
#include <stdio.h>
#include <malloc.h>

unsigned char *data = NULL;

Elf32_Ehdr *pElfHdr32 = NULL;
Elf64_Ehdr *pElfHdr64 = NULL;

static int is32(const unsigned char *p) {
  if (p) {
    return 1 == p[4] ? 1 : 0;
  }

  return -1;
}

static int is64(const unsigned char *p) {
  if (p) {
    return 2 == p[4] ? 1 : 0;
  }

  return -1;
}

static int isELF(const unsigned char *p) {
  if (p) {
    return 0x7f == p[0] && 'E' == p[1] && 'L' == p[2] && 'F' == p[3] ? 1 : 0;
  }

  return -1;
}

static unsigned char* get(const char *name) {
  FILE* f = fopen(name, "rb");
  if (f) {
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char* p = malloc(n);
    if (n != fread(p, 1, n, f)) {
      free(p);
    }

    fclose(f);
    return p;
  }

  return 0;
}

static Elf32_Ehdr* get_ehdr32(const unsigned char *p) {
  return (Elf32_Ehdr*)p;
}

static Elf64_Ehdr* get_ehdr64(const unsigned char *p) {
  return (Elf64_Ehdr*)p;
}

static Elf64_Shdr* get_shdr64(const unsigned char *p) {
  if (p) {
    Elf64_Ehdr *e = get_ehdr64(p);
    return (Elf64_Shdr*)(p + e->e_shoff);
  }
  return NULL;
}

static int show_magic64(Elf64_Ehdr *e) {
  if (e) {
    printf("  Magic: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  e->e_ident[0],  e->e_ident[1],  e->e_ident[2],  e->e_ident[3],
                  e->e_ident[4],  e->e_ident[5],  e->e_ident[6],  e->e_ident[7],
                  e->e_ident[8],  e->e_ident[9],  e->e_ident[10], e->e_ident[11],
                  e->e_ident[12], e->e_ident[13], e->e_ident[14], e->e_ident[15]);

    return 0;
  }

  return -1;
}

static int show_ehdrtype64(Elf64_Ehdr *e) {
  if (e) {
    switch (e->e_type) {
    case ET_NONE:
      printf("  Type: NONE (No file type)\n");
      break;
    case ET_REL:
      printf("  Type: REL (Relocatable file)\n");
      break;
    case ET_EXEC:
      printf("  Type: EXEC (Executable file)\n");
      break;
    case ET_DYN:
      printf("  Type: DYN (Shared object file)\n");
      break;
    default:
      printf("  Type: %d\n", e->e_type);
      return 1;
    }
    return 0;
  }
  return -1;
}

static int show_shdrtype64(Elf64_Shdr *s) {
  if (s) {
    switch (s->sh_type) {
    case SHT_NULL:
      printf("  Type: NULL\n");
      break;
    case SHT_PROGBITS:
      printf("  Type: PROGBITS\n");
      break;
    case SHT_SYMTAB:
      printf("  Type: SYMTAB\n");
      break;
    case SHT_STRTAB:
      printf("  Type: STRTAB\n");
      break;
    case SHT_NOTE:
      printf("  Type: NOTE\n");
      break;
    default:
      printf("  Type: %d\n", s->sh_type);
      return 1;
    }
    return 0;
  }
  return -1;
}

static int show_shdr64(Elf64_Shdr *s) {
  printf("SECTION HEADERS\n");
  printf("  Name: %d\n", s->sh_name);
  show_shdrtype64(s);
  return 0;
}


static int show(const unsigned char *p) {
  printf("32 BIT: %d\n", is32(p));
  printf("64 BIT: %d\n", is64(p));
  printf("ELF:    %d\n", isELF(p));

  if (is32(p)) {
    Elf32_Ehdr *e = get_ehdr32(p);
  } else if (is64(p)) {
    Elf64_Ehdr *e = get_ehdr64(p);
    show_magic64(e);
    show_ehdrtype64(e);
    Elf64_Shdr *s = get_shdr64(p);
    show_shdr64(s);
  }
  return 0;
}

static char* get_format(bfd *p) {
  if (bfd_check_format(p, bfd_archive))       return "BFD_ARCHIVE";
  else if (bfd_check_format(p, bfd_object))   return "BFD_OBJECT";
  else if (bfd_check_format(p, bfd_core))     return "BFD_CORE";

  return "BFD_UNK";
}

static void do_section(bfd *abfd, const char *name) {
  printf("-- %s\n", name);
  asection *osec = bfd_get_section_by_name(abfd, ".data");
  if (osec) {
    printf("got section\n");
    if ((bfd_section_flags (osec) & SEC_HAS_CONTENTS) == 0) {
      printf("no contents\n");
    } else {
      bfd_size_type size = bfd_section_size (osec);
      printf("section size: %lu\n", size);
    }
  }
}

int main() {
  bfd_vma start;
  enum bfd_architecture iarch;
  unsigned int imach;

  data = get("example");
  if (data) {
    show(data);
  }
	
  bfd* ibfd = bfd_openr("example", NULL);
  if (ibfd) {
    printf("size: %lu\n", bfd_get_size(ibfd));
    printf("format: %s\n", get_format(ibfd));
    printf("format: %s\n", bfd_format_string(ibfd->format));
    if (ibfd->sections == NULL) {
      printf("missing sections\n");
    }
    start = bfd_get_start_address (ibfd);
    printf("start: %lu\n", start);
    
    iarch = bfd_get_arch (ibfd);
    imach = bfd_get_mach (ibfd);
    printf("%d %d\n", iarch, imach);
    
    do_section(ibfd, ".data");
    do_section(ibfd, ".symtab");
    do_section(ibfd, ".strtab");
    do_section(ibfd, ".shstrtab");
    
    bfd_close(ibfd);
  }

  free(data);
  return 0;
}
