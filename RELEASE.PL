use v5.30.0;

use strict;
use warnings;

use Const::Fast;

use Path::Tiny qw/path/;
use JSON;
use File::pushd;

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
my $clone_path           = Path::Tiny->tempdir;
system qw/git clone/,
  'https://gitea.sergiotarxz.freemyip.com/sergiotarxz/mangareader/',
  $clone_path;

my @subs = (compile('x86_64'), compile('aarch64'));

my $arch    = shift;
my $release = request(
    POST => '/repos/sergiotarxz/mangareader/releases' => json => {
        body             => $description,
        name             => $title,
        tag_name         => $tag,
        target_commitish => 'main',
    }
);

print Data::Dumper::Dumper $release;
my $release_id = $release->{id};

for my $sub (@subs) {
    $sub->() if ref $sub eq 'CODE';
}

sub compile {
    my $arch                 = shift;
    my $flatpak_builder_file = 'me.sergiotarxz.openmg.json';
    my $app_id               = 'me.sergiotarxz.openmg';
    my $app_output_name      = path("openmg-$arch-$tag.flatpak")->absolute;
    system 'cp', '/usr/bin/qemu-aarch64',
            "$ENV{HOME}/.local/share/flatpak/runtime/org.gnome.Sdk/aarch64/master/active/files/bin/";

    my $push = pushd $clone_path;
    system 'flatpak-builder', '--force-clean', '--arch', $arch, '--install', '--user',
      'build', $flatpak_builder_file, $app_id
      and return 1;
    system 'flatpak', 'build-bundle', '--arch', $arch,
      path( $ENV{HOME} )->child('.local/share/flatpak/repo/'),
      $app_output_name, $app_id
      and return 1;
    return sub {
        print Data::Dumper::Dumper request( POST =>
"/repos/sergiotarxz/mangareader/releases/$release_id/assets?name=@{[$app_output_name->basename]}"
              => form => { attachment => { file => "" . $app_output_name } } );
    }
}

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
