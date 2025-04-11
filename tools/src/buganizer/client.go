// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package buganizer

import (
	//"context"
	"fmt"
	"io"
	"net/http"
	"strings"

	//"google.golang.org/api/discovery/v1"
	"cloud.google.com/go/auth/credentials"
	"cloud.google.com/go/auth/httptransport"
)

const baseUrl = "https://issuetracker.googleapis.com"

type Client struct {
	cloudClient *http.Client
}

func NewClient() (*Client, error) {
	options := httptransport.Options{
		DetectOpts: &credentials.DetectOptions{
			Scopes: []string{"https://www.googleapis.com/auth/buganizer", "https://www.googleapis.com/auth/userinfo.email",},
		},
	}
	cloudClient, err := httptransport.NewClient(&options)
	if err != nil {
		return nil, err
	}

	return &Client{
			cloudClient: cloudClient,
	}, nil
}

func (c Client) CreateIssue(issue NewIssueInfo) (int, error) {
	jsonString, err := issue.AsJSONString()
	if err != nil {
		return 0, err
	}

	endpoint := fmt.Sprintf("%s/v1/issues", baseUrl)
	resp, err := c.cloudClient.Post(endpoint, "application/json", strings.NewReader(jsonString))
	if err != nil {
		return 0, err
	}
	defer resp.Body.Close()
	fmt.Printf("Got response: %v\n", resp)
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return 0, err
	}
	fmt.Printf("Got body: %v\n", string(body[:]))
	return 0, nil
}

func (c Client) CloseIssue() {}

func (c Client) PostComment() {}

func (c Client) GetIssue(issue int) error {
	resp, err := c.cloudClient.Get(fmt.Sprintf("%s/v1/issues/%d", baseUrl, issue))
	if err != nil {
		return err
	}
	fmt.Printf("Got response: %v\n", resp)
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return err
	}
	fmt.Printf("Got body: %v\n", string(body[:]))
	return nil
}

/*type Client struct {
	service *discovery.Service
}

func NewClient(ctx context.Context) (*Client, error) {
	service, err := discovery.NewService(ctx)
	if err != nil {
		return nil, err
	}

	return &Client{
		service: service,
	}, nil
}

func (c Client) GetRest() error {
	resp, err := c.service.Apis.GetRest("issuetracker", "v1").Fields("schemas").Do()
	fmt.Printf("Resp: %v", resp)
	return err
}*/
