def Settings( **kwargs ):
  return {
    'flags': [ '-x', 'c++', '-Wall', '-Isrc', '-Iinclude', '--target=x86'],
  }