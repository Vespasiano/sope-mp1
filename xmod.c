#include <sys/stat.h>        /* For stat, chmod and umask.  */


static int char_to_mode_t (char *file, char *mode, gfc_charlen_type mode_len){

  int i;
  bool ugo[3];
  bool rwx[3];
  int set_mode, part;
  bool continue_clause = false;
  //idk
#ifndef __MINGW32__
  bool is_dir;
#endif
  mode_t mode_mask, file_mode, new_mode;
  struct stat stat_buf;
  if (mode_len == 0)
    return 1;

  if (mode[0] >= '0' && mode[0] <= '9')
    {
      unsigned fmode;
      if (sscanf (mode, "%o", &fmode) != 1)
        return 1;
      return chmod (file, (mode_t) fmode);
    }
  /* Read the current file mode. */
  if (stat (file, &stat_buf))
    return 1;
  file_mode = stat_buf.st_mode & ~S_IFMT;

  //idk
#ifndef __MINGW32__
  is_dir = stat_buf.st_mode & S_IFDIR;
#endif

  for (i = 0; i < mode_len; i++)
    {
      if (!continue_clause)
        {
          ugo[0] = false;
          ugo[1] = false;
          ugo[2] = false;

        }
      continue_clause = false; 
      rwx[0] = false;
      rwx[1] = false;
      rwx[2] = false;
      part = 0;
      set_mode = -1;
      for (; i < mode_len; i++)
        {
          switch (mode[i])
            {
            /* User setting: a[ll]/u[ser]/g[roup]/o[ther].  */
            case 'a':
              if (part != 0)
                return 1;
              ugo[0] = true;
              ugo[1] = true;
              ugo[2] = true;
              part = 1;
              break;

            case 'u':
              if (part != 0)
                return 1;
              ugo[0] = true;
              part = 1;
              break;

            case 'g':
              if (part != 0)
                return 1;
              ugo[1] = true;
              part = 1;
              break;

            case 'o':
              if (part != 0)
                return 1;
              ugo[2] = true;
              part = 1;
              break;

            /* Mode setting: =+-.  */
            case '=':
              if (part != 1)
                {
                  continue_clause = true;
                  i--;
                  part = 2;
                  goto clause_done;
                }
              set_mode = 1;
              part = 2;
              break;

            case '-':
              if (part != 1)
                {
                  continue_clause = true;
                  i--;
                  part = 2;
                  goto clause_done;
                }
              set_mode = 2;
              part = 2;
              break;

            case '+':
              if (part != 1)
                {
                  continue_clause = true;
                  i--;
                  part = 2;
                  goto clause_done;
                }
              set_mode = 3;
              part = 2;
              break;

            /* Permissions: rwx */
            case 'r':
              if (part != 2 && part != 3)
                return 1;
              rwx[0] = true;
              part = 3;
              break;

            case 'w':
              if (part != 2 && part != 3)
                return 1;
              rwx[1] = true;
              part = 3;
              break;

            case 'x':
              if (part != 2 && part != 3)
                return 1;
              rwx[2] = true;
              part = 3;
              break;

            default:
              return 1;
            }
        }
clause_done:
      if (part < 2)
        return 1;
      new_mode = 0;

      /* Read. */
      if (rwx[0])
        {
          if (ugo[0])
            new_mode |= S_IRUSR;
          if (ugo[1])
            new_mode |= S_IRGRP;
          if (ugo[2])
            new_mode |= S_IROTH;
        }
      /* Write.  */
      if (rwx[1])
        {
          if (ugo[0])
            new_mode |= S_IWUSR;
          if (ugo[1])
            new_mode |= S_IWGRP;
          if (ugo[2])
            new_mode |= S_IWOTH;
        }
      /* Execute. */
      if (rwx[2])
        {
          if (ugo[0])
            new_mode |= S_IXUSR;
          if (ugo[1])
            new_mode |= S_IXGRP;
          if (ugo[2])
            new_mode |= S_IXOTH;
        }


    if (set_mode == 1){

        /* Set '='.  */
        if ((ugo[0]))
          file_mode = (file_mode & ~(S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR))
                      | (new_mode & (S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR));
        if ((ugo[1]))
          file_mode = (file_mode & ~(S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP))
                      | (new_mode & (S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP));
        if ((ugo[2]))
          file_mode = (file_mode & ~(S_IROTH | S_IWOTH | S_IXOTH))
                      | (new_mode & (S_IROTH | S_IWOTH | S_IXOTH));
//idk
/*
#ifndef __VXWORKS__
        if (is_dir && rwxXstugo[5])
          file_mode |= S_ISVTX;
        else if (!is_dir)
          file_mode &= ~S_ISVTX;
#endif
*/

    }
    else if (set_mode == 2){

        /* Clear '-'.  */
        file_mode &= ~new_mode;
    }

    else if (set_mode == 3){

        /* Add '+'.  */
        file_mode |= new_mode;
    }
  }
  return chmod (file, file_mode);
}