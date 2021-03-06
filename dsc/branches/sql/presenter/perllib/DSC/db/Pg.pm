package DSC::db;

use DBI;
use POSIX;
use Time::HiRes; # XXX for debugging

use strict;

BEGIN {
	use Exporter   ();
	use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
	$VERSION     = 1.00;
	@ISA	 = qw(Exporter);
	@EXPORT      = qw();
	%EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
	@EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

$DSC::db::specific->{Pg} = {

key_type => 'VARCHAR', # postgres doesn't need max size

postconnect => sub {
    my ($dbh) = @_;
    $dbh->do("SET search_path TO $schema");
},

specific_init_db => sub {
    my ($dbh) = @_;

    $dbh->do('CREATE SEQUENCE seq_server_id');
    $dbh->do('ALTER TABLE server ALTER COLUMN server_id SET DEFAULT ' .
	'nextval(\'seq_server_id\')');

    $dbh->do('CREATE SEQUENCE seq_node_id');
    $dbh->do('ALTER TABLE node ALTER COLUMN node_id SET DEFAULT ' .
	'nextval(\'seq_node_id\')');
},

table_exists => sub {
    my ($dbh, $tabname) = @_;
    my $sth = $dbh->prepare_cached(
	"SELECT 1 FROM pg_tables WHERE tablename = ?");
    $sth->execute($tabname);
    my $result = scalar $sth->fetchrow_array;
    $sth->finish;
    return $result;
},

# returns a reference to an array of data table names
data_table_names => sub {
    my ($dbh) = @_;
    return $dbh->selectcol_arrayref("SELECT viewname FROM pg_views " .
	"WHERE schemaname = 'dsc' AND viewname LIKE 'dsc_%'");
},

# returns a reference to an array of data table index names
data_index_names => sub {
    my ($dbh) = @_;
    return $dbh->selectcol_arrayref("SELECT indexname FROM pg_indexes " .
	"WHERE schemaname = 'dsc' AND indexname LIKE 'dsc_%'");
},

#
# write 1-dimensional hash with time to table with 1 minute buckets
#
write_data => sub {
	# parameter $t is ignored.
	my ($dbh, $A, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $t (keys %$A) {
	    my $B = $A->{$t};
	    foreach my $k (keys %$B) {
		$dbh->pg_putline("$server_id\t$node_id\t$t\t$k\t$B->{$k}\n");
		$nl++;
	    }
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},

# write 1-dimensional hash without time to table with 1 day buckets
#
write_data2 => sub {
	my ($dbh, $href, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $k1 (keys %$href) {
	    $dbh->pg_putline("$server_id\t$node_id\t$t\t$k1\t$href->{$k1}\n");
	    $nl++;
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},

# write 2-dimensional hash without time to table with 1 day buckets
#
write_data3 => sub {
	my ($dbh, $href, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $k1 (keys %$href) {
		foreach my $k2 (keys %{$href->{$k1}}) {
		    $dbh->pg_putline("$server_id\t$node_id\t$t\t$k1\t$k2\t$href->{$k1}{$k2}\n");
		    $nl++;
		}
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},

#
# write 2-dimensional hash with time to table with 1 minute buckets
#
write_data4 => sub {
	# parameter $t is ignored.
	my ($dbh, $A, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	my ($B, $C);
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $t (keys %$A) {
	    $B = $A->{$t};
	    foreach my $k1 (keys %$B) {
		next unless defined($C = $B->{$k1});
		foreach my $k2 (keys %$C) {
		    $dbh->pg_putline("$server_id\t$node_id\t$t\t$k1\t$k2\t$C->{$k2}\n");
		    $nl++;
		}
	    }
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},
};

1;
