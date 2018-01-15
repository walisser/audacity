#!/usr/bin/python3

import os

sizeof = [
   ["short",2],
   ["int",4],
   ["long",4,8],
   ["long long",8],
   ["float",4],
   ["double",8],
   ["int16_t",2,4],
   ["uint16_t",2,4],
   ["int32_t",4,8],
   ["uint32_t",4,8],
   ["int64_t",8],
   ["uint64_t",8],
   ["size_t",4,8],
   ["off_t",4,8],
   ["void*",4,8],
   ["ssize_t",4,8],
   ["wchar_t",2,4],
]

os.mkdir('sizeof')

for typeConf in sizeof:
   key=typeConf[0]
   for size in typeConf[1:]:
      name=key.replace(' ', '_').replace('*','p');

      test=''
      if name.endswith('_t'):
         test+='#include <stdint.h>\n'
         test+='#include <stdio.h>\n'
         test+='#include <stdlib.h>\n'


      test+='static_assert(sizeof(%s)==%d, "");\n' % (key, size)
      config='CONFIG_H += SIZEOF_%s,%d\n' % \
         (name.upper(), size)
      #print(test)
      fh=open('sizeof/%s%d.h' % (name, size), 'w')
      fh.write(test)
      fh.close

      #print(config)
      fh=open('sizeof/%s%d.prf' % (name, size), 'w')
      fh.write(config)
      fh.close

fh=open('sizeof.prf','w')
for typeConf in sizeof:
   key=typeConf[0]
   #print(key)
   name=key.replace(' ', '_').replace('*','p')
   line=[]
   for size in typeConf[1:]:
      line.append('test(sizeof/%s%d)' % (name, size))
   line.append('fail(sizeof(%s))' % key)
   #print("|".join(line))
   fh.write("|".join(line)+'\n')
fh.close()
