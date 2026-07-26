package My::Suite::TypeEmail;

@ISA = qw(My::Suite);

return "Not run for embedded server" if $::opt_embedded_server;
bless { };
