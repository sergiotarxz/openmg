use v5.30.0;

use strict;
use warnings;

use Const::Fast;

use Path::Tiny qw/path/;
use JSON;

use Mojo::URL;
use Mojo::UserAgent;

my $tag         = $ARGV[0];
my $title       = $ARGV[1] // "Automated release without title $tag";
my $description = $ARGV[2] // '';

const my $config_file => "$ENV{HOME}/.config/openmg_releaser.json";

if ( !-f $config_file ) {
    die "No credentials in $config_file.";
}
my $config_file_contents = path($config_file)->slurp_utf8;
my $config               = decode_json($config_file_contents);
my $username             = $config->{username} // die "No user in config.";
my $token                = $config->{token}    // die "No token in config.";
my $host                 = $config->{host}     // die "No host in config.";
my $commit               = `git rev-parse HEAD`;
my $clone_url            = Path::Tiny->tempdir;

my $release = request(
    POST => '/repos/sergiotarxz/mangareader/releases' => json => {
        body             => $description,
        name             => $title,
        tag_name         => $tag,
        target_commitish => 'main',
    }
);
print Data::Dumper::Dumper $release;

sub request {
    my $method    = shift // die "No method passed.";
    my $endpoint  = shift // die "No endpoint passed.";
    my $body_type = shift // die "No body type passed.";
    my $body      = shift // die "No body passed.";
    my $ua        = Mojo::UserAgent->new();
    my $url       = Mojo::URL->new("https://$host/api/v1/$endpoint");
    $url->query( token => $token );
    say $url;
    my $tx = $ua->build_tx( $method => $url => {} => $body_type => $body );
    $ua->start($tx);
    my $response = $tx->result;
    say $response->code;
    say $response->message;
    return decode_json( $response->body );
}
