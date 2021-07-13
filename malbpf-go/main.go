package main

import (
	"bytes"
	"io"
	"net/http"

	"github.com/pkg/errors"
	"github.com/sirupsen/logrus"

	"github.com/DataDog/ebpf/manager"
)

var m = &manager.Manager{
	Probes: []*manager.Probe{
		&manager.Probe{
			Section:       "xdp/ingress",
			Ifindex:       2, // change this to the interface index connected to the internet
			XDPAttachMode: manager.XdpAttachModeSkb,
		},
	},
}

func main() {
	// Initialize the manager
	if err := m.Init(recoverAssets()); err != nil {
		logrus.Fatal(err)
	}

	// Start the manager
	if err := m.Start(); err != nil {
		logrus.Fatal(err)
	}

	logrus.Println("successfully started, head over to /sys/kernel/debug/tracing/trace_pipe")

	// Generate some network traffic to trigger the probe
	trigger()

	// Close the manager
	if err := m.Stop(manager.CleanAll); err != nil {
		logrus.Fatal(err)
	}
}

// recoverAssets - Recover ebpf asset
func recoverAssets() io.ReaderAt {
	buf, err := Asset("probe.o")
	if err != nil {
		logrus.Fatal(errors.Wrap(err, "couldn't find asset"))
	}
	return bytes.NewReader(buf)
}

// trigger - Generate some network traffic to trigger the probe
func trigger() {
	logrus.Println("Generating some network traffic to trigger the probes ...")
	_, _ = http.Get("https://www.google.com/")
}
