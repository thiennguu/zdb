Collecting Data via UDP/StatsD
===============================

To listen for samples via UDP, add a `listen_udp` block to your configuration
file.`metric-collectd` will listen for UDP packets on the specified port. 

    listen_udp {
      port 8125
      format statsd
    }


This is the list of valid stanzas within the `listen_udp` block:

<table>
  <thead>
    <tr>
      <th>Setting</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code><strong>port</strong></code></td>
      <td>The port on which the UDP server should be started</td>
    </tr>
    <tr>
      <td><code><strong>format</strong></code></td>
      <td>The expected input sample format ('statsd' or 'json')</td>
    </tr>
    <tr>
      <td><code><strong>rewrite</strong></code></td>
      <td>Rewrite the metric name or labels (see <a href="/documentation/configuration-file#rewrite">Configuration File</a>)</td>
    </tr>
  </tbody>
</table>

StatsD Protocol
---------------

StatsD is a very simple, text based protocol. For detailed information have a
look at the [Sample Formats](/documentation/sample-format) page.

When using the statsd protocol, each UDP packet must contain one or more lines.
Each line is expected to be in this format:

    <metricname>:<value>

A simple way to send in metrics from your command line if you have
metric-collectd running on port 8125 would be to use the `netcat` utility:

    $ echo "foo:1" | nc -u -w0 127.0.0.1 8125

This would insert the value "1" into the metric "foo".


Using metric labels with StatsD
-------------------------------

The statsd protocol itself does not support labels. However, the statsd
implementation in metric-collectd understands a common metric naming convention
that allows you to specify labels.

The line format for metrics with one label is:

    <metricname>{<label_key>=<label_value>}:<value>

To attach multiple labels, put them all into the curly braces separated by commas:

    <metricname>{<label1_key>=<label1_value>,<label2_key>=<label2_value>}:<value>

For example, if we want to sample the CPU utilization on multiple hosts into
the metric "cpu-utilization" with two label dimensions "hostname" and "datacenter",
our UDP packets could look like this:

_Insert value `0.642` with labels `hostname=machine83` and `datacenter=ams1` into metric `cpu-utilization`:_

    cpu-utilization{hostname=machine83,datacenter=ams1}:0.642

Batch insert
------------

You may include multiple lines in the same UDP packet. The lines may be
separated with `\n` or `\r\n`.

For example, to insert the values 23, 42 and 5 into the metrics "mymetric-one",
"mymetric-two" and "mymetric-three", you could send this UDP packet:

    mymetric-one:23\n
    mymetric-two:42\n
    mymetric-three:5\n

