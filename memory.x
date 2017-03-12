/*
 * $Id: memory.x,v 1.1 2004/03/15 00:13:27 heroine Exp $
 *
 * Boot block memory map for 68HC11 GNU tools
 */
MEMORY
{
  page0 (rwx) : ORIGIN = 0x0, LENGTH = 0x200
  text  (rx)  : ORIGIN = 0x0, LENGTH = 0x200
  data  (rwx) : ORIGIN = 0x0, LENGTH = 0x200
}
